#include "qualified_label_relation.h"

#include <numeric>
#include <vector>
#include <memory>

#include "qualified_local_state_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"

using namespace std;
using namespace fts;

namespace qdominance {

    QualifiedLabelRelation::QualifiedLabelRelation(const fts::FTSTask & fts_task,
                                 const std::vector<std::unique_ptr<QualifiedLocalStateRelation>> &sim) : num_labels(fts_task.get_num_labels()) {
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

        for (int i = 0; i < num_factors; ++i) {
            update(i, fts_task.get_factor(i), *(sim[i]));
        }
    }


    void QualifiedLabelRelation::dump_dominance(utils::LogProxy &) const {
        //TODO: implement
/*
        for (size_t l1 = 0; l1 < dominates_in.size(); ++l1) {
            for (size_t l2 = 0; l2 < dominates_in.size(); ++l2) {
                if (!dominates_in[l1][l2].is_none() && dominates_in[l2][l1] != dominates_in[l1][l2]) {
                   // log << l1 << " dominates " << l2 << " in " << dominates_in[l1][l2] << endl;
//                    log << g_operators[l1].get_name() << " dominates " << g_operators[l2].get_name() << endl;
                }
            }
        }
*/
    }



    bool QualifiedLabelRelation::update(const FTSTask & task, const std::vector<std::unique_ptr<QualifiedLocalStateRelation>> &sim) {
        bool changes = false;
        for (int i = 0; i < task.get_num_variables(); ++i) {
            changes |= update(i, task.get_factor(i), *(sim[i]));
        }
        return changes;
    }

// TODO (efficiency): This version is inefficient. It could be improved by iterating only the right transitions (see inside the loop)
    bool QualifiedLabelRelation::update(int i, const LabelledTransitionSystem &lts, const QualifiedLocalStateRelation &sim) {
        bool changes = false;
        for (int l2: lts.get_relevant_labels()) {
            for (int l1: lts.get_relevant_labels()) {
                if (l1 != l2 && simulates(l1, l2, i)) {
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
                            set_not_simulates(l1, l2, i);
                            changes = true;
                            break; //Stop checking trs of l1
                        }
                    }
                }
            }

            //Is l2 simulated by irrelevant_labels in lts?
            for (auto tr: lts.get_transitions_label(l2)) {
                if (simulated_by_irrelevant[l2][i] &&
                    !sim.simulates(tr.src, tr.target)) {
                    changes |= set_not_simulated_by_irrelevant(l2, i);
                    for (int l: lts.get_irrelevant_labels()) {
                        if (simulates(l, l2, i)) {
                            changes = true;
                            set_not_simulates(l, l2, i);
                        }
                    }
                    break;
                }
            }

            //Does l2 simulates irrelevant_labels in lts?
            if (simulates_irrelevant[l2][i]) {
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
                        simulates_irrelevant[l2][i] = false;
                        for (int l: lts.get_irrelevant_labels()) {
                            if (simulates(l2, l, i)) {
                                set_not_simulates(l2, l, i);
                                changes = true;
                            }
                        }
                        break;
                    }
                }
            }
        }

        return changes;
    }
}