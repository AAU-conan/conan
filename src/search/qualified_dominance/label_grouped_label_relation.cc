#include "label_grouped_label_relation.h"

#include <numeric>
#include <vector>
#include <memory>
#include <print>

#include "label_group_relation.h"
#include "sparse_local_state_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"

using namespace std;
using namespace fts;

namespace dominance {

    LabelGroupedLabelRelation::LabelGroupedLabelRelation(const fts::FTSTask & fts_task)
    : fts_task(fts_task) {
        for (size_t i = 0; i < fts_task.get_factors().size(); ++i) {
            const auto& lts = fts_task.get_factor(i);
            label_group_simulation_relations.emplace_back(lts, i);
        }
    }

    bool LabelGroupedLabelRelation::label_group_simulates(int factor, LabelGroup lg1, LabelGroup lg2) const {
        return label_group_simulation_relations.at(factor).simulates(lg1, lg2);
    }

    bool LabelGroupedLabelRelation::noop_simulates_label_group(int factor, LabelGroup lg) const {
        return label_group_simulation_relations.at(factor).noop_simulates(lg);
    }

    bool LabelGroupedLabelRelation::update(int factor, const FactorDominanceRelation& sim) {
        return label_group_simulation_relations.at(factor).update(sim);
    }

    void LabelGroupedLabelRelation::print_label_dominance() const {
        for (const auto& lgsr : label_group_simulation_relations) {
            std::println("Factor {}", lgsr.factor);
            for (const auto& [lg1, lg2] : lgsr.simulations) {
                std::println("{} simulates {}", lgsr.lts.label_group_name(lg1), lgsr.lts.label_group_name(lg2));
            }
            for (const auto& lg : lgsr.noop_simulations) {
                std::println("noop simulates {}", lgsr.lts.label_group_name(lg));
            }
            for (const auto& lg : lgsr.simulations_noop) {
                std::println("{} simulates noop", lgsr.lts.label_group_name(lg));
            }
        }
    }
}
