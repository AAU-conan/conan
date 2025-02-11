#include "dense_label_relation.h"
#include "all_none_factor_index.h"
#include "../factored_transition_system/fts_task.h"
#include "factor_dominance_relation.h"
#include "../plugins/plugin.h"

#include <cassert>

namespace dominance {
    bool DenseLabelRelation::simulates(int l1, int l2, int factor) const {
        assert (l1 >= 0);
        assert((size_t)l1 < dominates_in.size());
        assert (l2 >= 0);
        assert((size_t)l2 < dominates_in[l1].size());

        return dominates_in[l1][l2].contains(factor);
    }

    bool DenseLabelRelation::noop_simulates(int l, int factor) const {
        assert(l >= 0);
        assert((size_t)l < dominated_by_noop_in.size());

        return dominated_by_noop_in[l].contains(factor);
    }

    void DenseLabelRelation::set_not_simulates(int l1, int l2, int lts) {
        //std::cout << "Not simulates: " << l1 << " to " << l2 << " in " << lts << std::endl;
        dominates_in[l1][l2].remove(lts);
    }

    bool DenseLabelRelation::set_not_simulated_by_irrelevant(int l, int lts) {
        //std::cout << "Not simulated by irrelevant: " << l << " in " << lts << std::endl;

        //Returns if there were changes in dominated_by_noop_in
        simulated_by_irrelevant[l][lts] = false;
        return dominated_by_noop_in[l].remove(lts);
    }

    int DenseLabelRelation::get_num_labels() const {
        return num_labels;
    }

    bool DenseLabelRelation::label_dominates_label_in_all_other(int factor, int l1, int l2) const {
        return dominates_in[l1][l2].contains_all_except(factor);
    }

    bool DenseLabelRelation::noop_simulates_label_in_all_other(int factor, int l) const {
        return dominated_by_noop_in[l].contains_all_except(factor);
    }

    bool DenseLabelRelation::update_factor(int factor, const FactorDominanceRelation& sim) {
        bool changes = false;
        const fts::LabelledTransitionSystem& lts = fts_task.get_factor(factor);
        for (fts::LabelGroup lg_2: lts.get_relevant_label_groups()) {
            for (int l2 : lts.get_labels(lg_2)) {
                for (fts::LabelGroup lg_1: lts.get_relevant_label_groups()) {
                    for (int l1 : lts.get_labels(lg_1)) {

                        if (l1 != l2 && simulates(l1, l2, factor)) {
                            //std::log << "Check " << l1 << " " << l2 << std::endl;
                            //std::log << "Num transitions: " << lts.get_transitions_label(l1).size()
                            //		    << " " << lts.get_transitions_label(l2).size() << std::endl;
                            //Check if it really simulates
                            //For each transition s--l2-->t, and every label l1 that dominates
                            //l2, exist s--l1-->t', t <= t'?
                            for (const auto &tr: lts.get_transitions_label(l2)) {
                                bool found = false;
                                //TODO (efficiency): for(auto tr2 : lts.get_transitions_for_label_src(l1, tr.src)){
                                for (const auto &tr2: lts.get_transitions_label(l1)) {
                                    if (tr2.src == tr.src &&
                                        sim.simulates(tr2.target, tr.target)) {
                                        found = true;
                                        break; //Stop checking this tr
                                        }
                                }
                                if (!found) {
                                    //std::log << "Not sim " << l1 << " " << l2 << " " << i << std::endl;
                                    set_not_simulates(l1, l2, factor);
                                    changes = true;
                                    break; //Stop checking trs of l1
                                }
                            }
                        }
                    }
                }

                //Is l2 simulated by irrelevant_labels in lts?
                for (auto tr: lts.get_transitions_label(l2)) {
                    if (simulated_by_irrelevant[l2][factor] &&
                        !sim.simulates(tr.src, tr.target)) {
                        changes |= set_not_simulated_by_irrelevant(l2, factor);
                        for (int l: lts.get_irrelevant_labels()) {
                            if (simulates(l, l2, factor)) {
                                changes = true;
                                set_not_simulates(l, l2, factor);
                            }
                        }
                        break;
                        }
                }

                //Does l2 simulates irrelevant_labels in lts?
                if (simulates_irrelevant[l2][factor]) {
                    for (int s = 0; s < lts.size(); s++) {
                        bool found = false;
                        for (const auto &tr: lts.get_transitions_label(l2)) {
                            if (tr.src == s && sim.simulates(tr.target, tr.src)) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            //log << "Not simulates irrelevant: " << l2  << " in " << i << endl;
                            simulates_irrelevant[l2][factor] = false;
                            for (int l: lts.get_irrelevant_labels()) {
                                if (simulates(l2, l, factor)) {
                                    set_not_simulates(l2, l, factor);
                                    changes = true;
                                }
                            }
                            break;
                        }
                    }
                }
            }
        }

        return changes;
    }

    DenseLabelRelation::DenseLabelRelation(const fts::FTSTask & fts_task) : LabelRelation(fts_task) {
        int num_factors = fts_task.get_num_variables();
        num_labels = fts_task.get_num_labels();

        simulates_irrelevant.resize(num_labels);
        simulated_by_irrelevant.resize(num_labels);
        for (int i = 0; i < num_labels; i++) {
            simulates_irrelevant[i].resize(num_factors, true);
            simulated_by_irrelevant[i].resize(num_factors, true);
        }

        dominates_in.resize(num_labels);
        dominated_by_noop_in.resize(num_labels, AllNoneFactorIndex::all_factors());
        for (size_t l1 = 0; l1 < dominates_in.size(); ++l1) {
            dominates_in[l1].resize(num_labels, AllNoneFactorIndex::all_factors());
            for (size_t l2 = 0; l2 < dominates_in[l1].size(); ++l2) {
                if (fts_task.get_label_cost(l1) > fts_task.get_label_cost(l2)) {
                    dominates_in[l1][l2] = AllNoneFactorIndex::no_factors();
                }
            }
        }
    }

    using LabelGroupedLabelRelationFactory = LabelRelationFactoryImpl<DenseLabelRelation>;
    class DenseLabelRelationFactoryFeature final : public plugins::TypedFeature<LabelRelationFactory, LabelGroupedLabelRelationFactory> {
    public:
        DenseLabelRelationFactoryFeature() : TypedFeature("dense_lr") {
            document_title("Dense Label Relation");
            document_synopsis("Stores the label relation in a dense matrix with compact factors");
        }

        [[nodiscard]] std::shared_ptr<LabelGroupedLabelRelationFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
            utils::unused_variable(opts);
            return plugins::make_shared_from_arg_tuples<LabelGroupedLabelRelationFactory>();
        }
    };
    static plugins::FeaturePlugin<DenseLabelRelationFactoryFeature> _dense_plugin;
}