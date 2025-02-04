#include "qualified_ld_simulation.h"

#include "../factored_transition_system/fts_task.h"
#include "label_grouped_label_relation.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

using std::vector;
using merge_and_shrink::TransitionSystem;

using namespace dominance;

namespace dominance {
    std::unique_ptr<FactoredDominanceRelation> IncrementalLDSimulation::compute_dominance_relation(const fts::FTSTask& task) {
        return compute_ld_simulation(task, log);
    }

    std::unique_ptr<FactoredDominanceRelation> IncrementalLDSimulation::compute_ld_simulation(const fts::FTSTask &task, utils::LogProxy &log) {
        utils::Timer t;
        std::unique_ptr<LabelRelation> label_relation = std::make_unique<LabelGroupedLabelRelation>(task);

        std::vector<std::unique_ptr<FactorDominanceRelation>> local_relations;
        local_relations.reserve(task.get_num_variables());

        for (const auto &[factor, lts] : std::views::enumerate(task.get_factors())) {
            local_relations.push_back(factor_dominance_relation_factory->create(*lts));
            label_relation->update_factor(static_cast<int>(factor), *local_relations.back()); // Make sure to update the label relation at least
            // once per factor
        }

        log << "Initialize qualified label dominance: " << task.get_num_labels()
            << " labels " << task.get_num_variables() << " systems." << std::endl;

        int total_size = 0, max_size = 0, total_trsize = 0, max_trsize = 0;
        for (const auto &lts : task.get_factors()) {
            max_size = std::max(max_size, lts->size());
            max_trsize = std::max(max_trsize, lts->num_transitions());
            total_size += lts->size();
            total_trsize += lts->num_transitions();
        }
        log << "Compute LDSim on " << task.get_num_variables() << " LTSs."
            << " Total factor size: " << total_size
            << ", total trsize: " << total_trsize << ", max factor size: " << max_size
            << ", max trsize: " << max_trsize << std::endl;

        log << "Init LDSim in " << t() << ":" << std::endl << std::flush;
        bool changes;
        do {
            changes = false;
            for (auto [factor, local_relation] : std::views::enumerate(local_relations)) {
                changes |= update_local_relation(static_cast<int>(factor), *local_relation, *label_relation);
                if (changes)
                    changes |= label_relation->update_factor(factor, *local_relation);
            }
            log << changes << " " << t() << std::endl;
        } while (changes);
        log << std::endl << "LDSimulation finished: " << t() << std::endl;

#ifndef NDEBUG
        for (const auto& sim : local_relations) {
            sim->dump(log);
        }
        log << "Label relation: " << std::endl;
        label_relation->dump(log);
#endif

        return std::make_unique<FactoredDominanceRelation>(std::move(local_relations), label_relation);
    }

    bool labels_simulate_labels(int factor, const std::unordered_set<int>& l1s, const std::vector<int>& l2s,
        bool include_noop, const LabelRelation& label_relation) {
        return std::ranges::all_of(l2s, [&](const auto& l2) {
            return (include_noop && label_relation.noop_simulates_label_in_all_other(factor, l2)) || std::ranges::any_of(l1s, [&](const auto& l1) {
                return label_relation.label_dominates_label_in_all_other(factor, l1, l2);
            });
        });
    }

    bool update_pairs(int factor, FactorDominanceRelation& local_relation,
        const LabelRelation& label_relation) {
        // Remove all simulation pairs (s, t) where it is not the case that
        // ∀s -lg-> s'( ∃t -lg'-> t' s.t. s' simulates t' and lg' simulates lg in all other factors or t simulates s' and noop simulates lg in all other factors)
        const auto& lts = local_relation.get_lts();
        return local_relation.removeSimulations([&](int t, int s) {
#ifndef NDEBUG
            // std::println("Checking {} <= {}", lts.state_name(s), lts.state_name(t));
#endif
            const auto& s_transitions = lts.get_transitions(s);
            const auto& t_transitions = lts.get_transitions(t);
            return !std::ranges::all_of(s_transitions, [&](const LTSTransition& s_tr) {
#ifndef NDEBUG
                // std::println("    {} --{}-> {}", lts.state_name(s_tr.src), lts.label_group_name(s_tr.label_group), lts.state_name(s_tr.target));
#endif
                if (!lts.is_relevant_label_group(s_tr.label_group)) {
                    return true;
                }

                std::unordered_set<int> t_labels;
                for (const LTSTransition& t_tr : t_transitions) {
                    if (local_relation.simulates(t_tr.target, s_tr.target)) {
                        for (const auto& l : lts.get_labels(t_tr.label_group)) {
                            t_labels.insert(l);
                        }
                    }
                }

                return labels_simulate_labels(factor, t_labels, lts.get_labels(s_tr.label_group), local_relation.simulates(t, s_tr.target), label_relation);
#ifndef NDEBUG
                // std::println("        {0} --noop-> {0} does {1}simulate", lts.state_name(t), noop_simulates_tr(t, s_tr, label_relation)? "" : "not ");
#endif
            });
        });
    }

    bool update_local_relation(int factor, FactorDominanceRelation& local_relation,
        const LabelRelation& label_relation) {
        bool any_changes = false;
        bool changes = false;
        do {
            changes = update_pairs(factor, local_relation, label_relation);
            any_changes |= changes;
        } while (changes);
#ifndef NDEBUG
        // std::println("{} simulations: {}", factor, boost::algorithm::join(std::views::transform(simulations, [&](const auto& p) { return std::format("{} <= {}", lts.state_name(p.second), lts.state_name(p.first));}) | std::ranges::to<std::vector>(), ", "));
#endif
        return any_changes;
    }


    class IncrementalLDSimulationFeature : public plugins::TypedFeature<DominanceAnalysis, IncrementalLDSimulation> {
    public:
        IncrementalLDSimulationFeature() : TypedFeature("incremental_ld_simulation") {
            document_title("Incremental LD Simulation");

            document_synopsis("Incremental version of LD simulation" //TODO (doc): Reference other relevant papers
            );
            document_language_support("action costs", "supported");
            document_language_support("conditional effects", "not supported");
            document_language_support("axioms", "not supported");

            add_option<std::shared_ptr<FactorDominanceRelationFactory>>("fdr",
                                                       "The data structure to store the factor dominance relation",
                                                       "sparse_fdr()");
            add_option<std::shared_ptr<LabelRelationFactory>>("lr",
                                                       "The data structure to store the label relation",
                                                       "grouped_lr()");

            utils::add_log_options_to_feature(*this);
        }


        virtual std::shared_ptr<IncrementalLDSimulation> create_component(
            const plugins::Options& opts,
            const utils::Context&) const override {
            return plugins::make_shared_from_arg_tuples<IncrementalLDSimulation>(
                utils::get_log_arguments_from_options(opts),
                opts.get<std::shared_ptr<FactorDominanceRelationFactory>>("fdr"),
                opts.get<std::shared_ptr<LabelRelationFactory>>("lr"));
        }
    };

    static plugins::FeaturePlugin<IncrementalLDSimulationFeature> _plugin;
}
