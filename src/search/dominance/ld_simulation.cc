#include "ld_simulation.h"

#include "../factored_transition_system/fts_task.h"
#include "label_relation.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

using std::vector;
using merge_and_shrink::TransitionSystem;

namespace dominance {
    std::unique_ptr<StateDominanceRelation> LDSimulation::compute_dominance_relation(const fts::FTSTask &task) {
        return compute_ld_simulation(task, log);
    }

    std::unique_ptr<StateDominanceRelation> LDSimulation::compute_ld_simulation(const fts::FTSTask & task, utils::LogProxy & log) {
        utils::Timer t;

        std::vector<std::unique_ptr<FactorDominanceRelation>> local_relations;
        local_relations.reserve(task.get_num_variables());
        for (const auto & lts: task.get_factors()) {
            local_relations.push_back(factor_dominance_relation_factory->create(*lts));
        }

        log << "Initialize label dominance: " << task.get_num_labels() << " labels " << task.get_num_variables() << " systems." << std::endl;

        std::unique_ptr<LabelRelation> label_relation = label_relation_factory->create(task);
        // Label relation is updated once before first iteration of local relation updates
        update_label_relation(*label_relation, task, local_relations);

        int total_size = 0, max_size = 0, total_trsize = 0, max_trsize = 0;
        for (const auto & lts: task.get_factors()) {
            max_size = std::max(max_size, lts->size());
            max_trsize = std::max(max_trsize, lts->num_transitions());
            total_size += lts->size();
            total_trsize += lts->num_transitions();
        }
        log << "Compute LDSim on " << task.get_num_variables() << " LTSs."
                  << " Total factor size: " << total_size
                  << ", total trsize: " << total_trsize
                  << ", max factor size: " << max_size
                  << ", max trsize: " << max_trsize
                  << std::endl;

        log << "Init LDSim in " << t() << ":" << std::flush;
        do {
            for (int i = 0; i < static_cast<int>(local_relations.size()); i++) {
                update_local_relation(i, task, *label_relation, *(local_relations[i]));
            }
            log << " " << t() << std::flush;
        } while (update_label_relation(*label_relation, task, local_relations));
        log << std::endl << "LDSimulation finished: " << t() << std::endl;

#ifndef NDEBUG
        for (const auto& [factor, sim] : std::views::enumerate(local_relations)) {
            sim->dump(log, task.get_factor(factor));
        }
        log << "Label relation: " << std::endl;

        label_relation->dump(log, task);
#endif
        return std::make_unique<StateDominanceRelation>(std::move(local_relations), label_relation);
    }

    bool update_local_relation(int lts_id, const fts::FTSTask& fts_task, const LabelRelation& label_dominance,
                               FactorDominanceRelation& local_relation) {
        bool changes = true;
        bool any_changes = false;
        const fts::LabelledTransitionSystem& lts = fts_task.get_factor(lts_id);
        while (changes) {
            changes = local_relation.remove_simulations_if([&](int t, int s) {
                //log << "Checking states " << lts->name(s) << " and " << lts->name(t) << endl;
                //Check if really t simulates s
                //for each transition s--l->s':
                // a) with noop t >= s' and l dominated by noop?
                // b) exist t--l'-->t', t' >= s' and l dominated by l'?
                return lts.applyPostSrc(s, [&](const auto &trs) {
                    //log << "Checking transition " << s << " to " << trs.target << std::endl;

                    const std::vector<int> &labels_trs = lts.get_labels(trs.label_group);
                 //   assert(!labels_trs.empty());
                    for (int labels_tr : labels_trs) {
                        //log << "Checking label " << labels_trs[i] << " to " << trs.target << std::endl;
                        if (local_relation.simulates(t, trs.target) && label_dominance.noop_simulates_label_in_all_other(lts_id, fts_task, labels_tr)) {
                            continue;
                        }
                        bool found =
                                lts.applyPostSrc(t, [&](const auto &trt) {
                                    if (local_relation.simulates(trt.target, trs.target)) {
                                        const std::vector<int> &labels_trt = lts.get_labels(trt.label_group);
                                        for (int label_trt: labels_trt) {
                                            if (label_dominance.label_dominates_label_in_all_other(lts_id, fts_task, label_trt, labels_tr)) {
                                                return true;
                                            }
                                        }
                                    }
                                    return false;
                                });

                        if (!found) {
                            return true;
                        }
                    }

                    return false;
                });
            });
            any_changes |= changes;
        }
        return any_changes;
    }

    bool update_label_relation(LabelRelation& label_relation, const fts::FTSTask & task, const std::vector<std::unique_ptr<FactorDominanceRelation>> &sim) {
        bool changes = false;
        for (int i = 0; i < task.get_num_variables(); ++i) {
            changes |= label_relation.update_factor(i, task, *(sim[i]));
        }
        return changes;
    }



    LDSimulation::LDSimulation(utils::Verbosity verbosity, std::shared_ptr<FactorDominanceRelationFactory> factor_dominance_relation_factory, std::shared_ptr<LabelRelationFactory> label_relation_factory) :
            log(utils::get_log_for_verbosity(verbosity)), factor_dominance_relation_factory(std::move(factor_dominance_relation_factory)), label_relation_factory(std::move(label_relation_factory)) {
    }


    class LDSimulationFeature
            : public plugins::TypedFeature<DominanceAnalysis, LDSimulation> {
    public:
        LDSimulationFeature() : TypedFeature("ld_simulation") {
            document_title("LDSimulation");

            document_synopsis(
                    "This dominance analysis method implements the algorithm described in the following "
                    "paper:" + utils::format_conference_reference(
                            {"{\'A}lvaro Torralba", "J\"org Hoffmann"},
                            "Simulation-Based Admissible Dominance Pruning",
                            "https://homes.cs.aau.dk/~alto/papers/ijcai15.pdf",
                            "Proceedings of the 24th International Joint Conference on Artificial Intelligence (IJCAI'15)",
                            "1689-1695",
                            "AAAI Press",
                            "2015") + "\n"//TODO (doc): Reference other relevant papers
            );
            document_language_support("action costs", "supported");
            document_language_support("conditional effects", "not supported");
            document_language_support("axioms", "not supported");

            add_option<std::shared_ptr<FactorDominanceRelationFactory>>("fdr",
                                                       "The data structure to store the factor dominance relation",
                                                       "dense_fdr()");
            add_option<std::shared_ptr<LabelRelationFactory>>("lr",
                                                       "The data structure to store the label relation",
                                                       "dense_lr()");

            utils::add_log_options_to_feature(*this);
        }


        virtual std::shared_ptr<LDSimulation> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {

            return plugins::make_shared_from_arg_tuples<LDSimulation>(
                    utils::get_log_arguments_from_options(opts),
                    opts.get<std::shared_ptr<FactorDominanceRelationFactory>>("fdr"),
                    opts.get<std::shared_ptr<LabelRelationFactory>>("lr"));
        }
    };

static plugins::FeaturePlugin<LDSimulationFeature> _plugin;

}