#ifndef MERGE_AND_SHRINK_LABEL_RELATION_NOOP_H
#define MERGE_AND_SHRINK_LABEL_RELATION_NOOP_H

#include "label_relation.h"

#include <vector>

namespace fts {
    class FTSTask;
}

namespace dominance {
    class AllNoneFactorIndex;

    /*
     * Label relation represents the preorder relations on labels that occur in a set of LTS
     */
    class NoopLabelRelation final : public LabelRelation {
        std::vector<AllNoneFactorIndex> dominated_by_noop_in;

    public:
        [[nodiscard]] bool label_dominates_label_in_all_other(int factor, const fts::FTSTask& fts_task, int l1, int l2) const override;

        [[nodiscard]] bool noop_simulates_label_in_all_other(int factor, const fts::FTSTask& fts_task, int l) const override;

        bool set_not_simulated_by_irrelevant(int l, int lts);
        bool update_factor(int factor, const fts::FTSTask& fts_task, const FactorDominanceRelation& sim) override;

        explicit NoopLabelRelation(const fts::FTSTask & fts_task);
    };
}

#endif
