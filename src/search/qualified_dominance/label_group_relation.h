#ifndef LABEL_GROUP_RELATION_H
#define LABEL_GROUP_RELATION_H

#include "../factored_transition_system/fts_task.h"

namespace qdominance {
    class QualifiedLabelRelation;
}

namespace qdominance {
    class LabelGroupSimulationRelation {
    private:
        // For each pair of label groups, whether the first simulates the second in all other transition systems
        [[nodiscard]] bool label_group_simulates_compute(fts::LabelGroup lg1, fts::LabelGroup lg2, const QualifiedLabelRelation& label_relation) const;

        // For each label group, whether it is simulated by noop in all other transition systems
        [[nodiscard]] bool label_group_simulated_by_noop_compute(fts::LabelGroup lg, const QualifiedLabelRelation& label_relation) const;
    public:
        const fts::LabelledTransitionSystem& lts;
        int factor;

        // for lg1 and lg2, does lg1 simulate lg2 in all other transition systems. 0: no, 1: yes, -1: not computed
        mutable std::vector<std::vector<int>> relation;

        mutable std::vector<int> simulated_by_noop_cache;

        explicit LabelGroupSimulationRelation(const fts::LabelledTransitionSystem& lts, int factor)
            : lts(lts), factor(factor), simulated_by_noop_cache(lts.get_num_label_groups(), -1) {
            relation.resize(lts.get_num_label_groups(), std::vector<int>(lts.get_num_label_groups(), -1));
        }

        // Computes whether lg1 simulates lg2 in all other transition systems, and caches the result
        [[nodiscard]] bool simulates(fts::LabelGroup lg1, fts::LabelGroup lg2, const QualifiedLabelRelation& label_relation) const {
            // Check if the value is already computed
            if (relation.at(lg1.group).at(lg2.group) == -1) {
                // If not, actually compute the value
                relation.at(lg1.group).at(lg2.group) = label_group_simulates_compute(lg1, lg2, label_relation);
            }
            return relation.at(lg1.group).at(lg2.group) == 1;
        }

        // Computes whether lg is simulated by noop in all other transition systems
        [[nodiscard]] bool simulated_by_noop(fts::LabelGroup lg, const QualifiedLabelRelation& label_relation) const {
            // Check if the value is already computed
            if (simulated_by_noop_cache.at(lg.group) == -1) {
                // If not, actually compute the value
                simulated_by_noop_cache.at(lg.group) = label_group_simulated_by_noop_compute(lg, label_relation);
            }
            return simulated_by_noop_cache.at(lg.group);
        }

        // l1 no longer simulates l2, invalidate the cache for their associated label groups
        void invalidate_label_cache(int l1, int l2) {
            relation.at(lts.get_group_label(l1).group).at(lts.get_group_label(l2).group) = -1;
        }

        // l is no longer simulated by noop, invalidate the cache for its associated label group
        void invalidate_noop_cache(int l) {
            // Actually, if one label is not simulated by noop, the label group is not simulated by noop
            simulated_by_noop_cache.at(lts.get_group_label(l).group) = 0;
        }
    };
}



#endif //LABEL_GROUP_RELATION_H
