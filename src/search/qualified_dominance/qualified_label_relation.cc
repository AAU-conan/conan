#include "qualified_label_relation.h"

#include <numeric>
#include <vector>
#include <memory>
#include <print>

#include "label_group_relation.h"
#include "qualified_local_state_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"

using namespace std;
using namespace fts;

namespace qdominance {

    QualifiedLabelRelation::QualifiedLabelRelation(const fts::FTSTask & fts_task)
    : fts_task(fts_task) {
        for (int i = 0; i < fts_task.get_factors().size(); ++i) {
            const auto& lts = fts_task.get_factor(i);
            label_group_simulation_relations.emplace_back(lts, i);
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

    bool QualifiedLabelRelation::label_group_simulates(int factor, LabelGroup lg1, LabelGroup lg2) const {
        return label_group_simulation_relations.at(factor).simulates(lg1, lg2);
    }

    bool QualifiedLabelRelation::noop_simulates_label_group(int factor, LabelGroup lg) const {
        return label_group_simulation_relations.at(factor).noop_simulates(lg);
    }

    bool QualifiedLabelRelation::update(int factor, const QualifiedLocalStateRelation& sim) {
        std::println("Updating {} label group relation", factor);
        return label_group_simulation_relations.at(factor).update(sim);
    }
}
