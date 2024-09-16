#include "qualified_label_relation.h"

#include <numeric>

#include "qualified_local_state_relation.h"
#include "qualified_factored_dominance_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../utils/logging.h"
#include "../merge_and_shrink/labels.h"
#include "../factored_transition_system/label_map.h"

using namespace std;
using namespace fts;

namespace qdominance {

    QualifiedLabelRelation::QualifiedLabelRelation(const fts::FTSTask & fts_task,
                                 const std::vector<std::unique_ptr<QualifiedLocalStateRelation>> &sim) : num_labels(fts_task.get_num_labels()) {
        int num_factors = fts_task.get_num_variables();
        num_labels = fts_task.get_num_labels();

        dominates_in.resize(num_labels);
        dominated_by_noop_in.resize(num_labels, std::vector(num_factors, QualifiedFormula::tt()));
        for (size_t l1 = 0; l1 < dominates_in.size(); ++l1) {
            dominates_in[l1].resize(num_labels);
            for (size_t l2 = 0; l2 < dominates_in[l1].size(); ++l2) {
                if (fts_task.get_label_cost(l1) > fts_task.get_label_cost(l2)) {
                    dominates_in[l1][l2].resize(num_factors, QualifiedFormula::ff());
                } else {
                    dominates_in[l1][l2].resize(num_factors, QualifiedFormula::tt());
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

    void QualifiedLabelRelation::dump(utils::LogProxy &log) const {
        for (size_t l = 0; l < dominates_in.size(); ++l) {
            //if (labels->is_label_reduced(l)) log << "reduced";
            if (l < 10) {
                log << "l" << l << ": ";
                dump(log, l);
            } else {
                log << "l" << l << ":";
                dump(log, l);
            }
        }
    }

    void QualifiedLabelRelation::dump(utils::LogProxy &, int ) const {
        //log << "Dump l: " << label << "; " << " Dominated by noop: " << dominated_by_noop_in[label] << ", labels: ";

/*        for (size_t l2 = 0; l2 < dominates_in[label].size(); ++l2) {
            if (dominates_in[l2][label] >= 0 && dominates_in[l2][label] <= 9) log << " ";
            log << dominates_in[l2][label] << " ";
        }*/
        //log << endl;
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
        using QF = QualifiedFormula;
        bool changes = false;
        for (int l1 = 0; l1 < num_labels; ++l1) {
            for (int l2 = 0; l2 < num_labels; ++l2) {
                if (l1 != l2 && qualified_simulates(l2, l1, i) != QualifiedFormula::ff())  {
                    // Construct formula for when l2 simulates l1 in i
                    std::vector<QualifiedFormula> l2_simulates_l1;
                    for (const auto &tr1: lts.get_transitions_label(l1)) {
                        std::vector<QualifiedFormula> l2_simulates_l1_tr;
                        for (const auto &tr2: lts.get_transitions_label(l2)) {
                            if (tr2.src == tr1.src) {
                                QF sim_next = sim.simulates_under(tr2.target, tr1.target);
                                if (sim_next != QF::ff()) {
                                    l2_simulates_l1_tr.push_back(QF::X(QF::G(sim_next)));
                                }
                            }
                        }
                        l2_simulates_l1.push_back(QualifiedFormula::Or(l2_simulates_l1_tr));
                    }
                    changes |= set_dominates_in(l2, l1, i, QualifiedFormula::And(l2_simulates_l1));
                }
            }
            std::vector<QualifiedFormula> noop_simulates_l1;
            for (const auto &tr: lts.get_transitions_label(l1)) {
                noop_simulates_l1.push_back(sim.simulates_under(tr.src, tr.target));
            }
            changes |= set_dominated_by_noop_in(l1, i, QualifiedFormula::And(noop_simulates_l1));
        }

        return changes;
    }
}