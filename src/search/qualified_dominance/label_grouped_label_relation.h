#ifndef LABEL_GROUPED_LABEL_RELATION_H
#define LABEL_GROUPED_LABEL_RELATION_H

#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <memory>
#include <generator>
#include <print>

#include "../dominance/all_none_factor_index.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "label_group_relation.h"
#include "../dominance/label_relation.h"


using namespace dominance;

namespace fts {
    class FTSTask;
    class LabelledTransitionSystem;
    class LabelMap;
}

namespace utils {
    class LogProxy;
}

namespace dominance {
    using TransitionIndex = size_t;
    class FactorDominanceRelation;

    /*
     * Label relation represents the preorder relations on labels that
     * occur in a set of LTS
     */
    class LabelGroupedLabelRelation final : public LabelRelation {
        std::vector<LabelGroupSimulationRelation> label_group_simulation_relations;

    public:
        explicit LabelGroupedLabelRelation(const fts::FTSTask& fts_task);

        bool update_factor(int factor, const FactorDominanceRelation& sim) override;

        void print_label_dominance() const;

        [[nodiscard]] bool label_group_simulates(int factor, fts::LabelGroup lg1, fts::LabelGroup lg2) const;
        [[nodiscard]] bool noop_simulates_label_group(int factor, fts::LabelGroup lg) const;


        [[nodiscard]] int get_num_labels() const {
            return fts_task.get_num_labels();
        }

        [[nodiscard]] bool label_dominates_label_in_all_other(const int factor, const int l1, const int l2) const override;

        [[nodiscard]] bool noop_simulates_label_in_all_other(const int factor, const int l) const override;
    };
}


#endif
