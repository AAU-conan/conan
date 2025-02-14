#ifndef DOMINANCE_DENSE_LABEL_RELATION_H
#define DOMINANCE_DENSE_LABEL_RELATION_H

#include "label_relation.h"

#include <vector>

namespace dominance {
    class AllNoneFactorIndex;

    class DenseLabelRelation : public LabelRelation {
        // Matrix factors where l1 simulates l2
        std::vector<std::vector<AllNoneFactorIndex> > dominates_in;

        //Indicates whether labels are dominated by noop or other irrelevant
        //variables in theta
        std::vector<std::vector<bool> > simulated_by_irrelevant;
        std::vector<std::vector<bool> > simulates_irrelevant;

        std::vector<AllNoneFactorIndex> dominated_by_noop_in;

        //Returns true if l1 simulates l2 in lts
        [[nodiscard]] bool simulates(int l1, int l2, int factor) const;

        [[nodiscard]] bool noop_simulates(int l, int factor) const;


        inline void set_not_simulates(int l1, int l2, int lts);

        inline bool set_not_simulated_by_irrelevant(int l, int lts);

    public:

        [[nodiscard]] bool label_dominates_label_in_all_other(int factor, const fts::FTSTask& fts_task, int l1, int l2) const override;

        [[nodiscard]] bool noop_dominates_label_in_all_other(int factor, const fts::FTSTask& fts_task, int l) const override;

        bool update_factor(int factor, const fts::FTSTask& fts_task, const FactorDominanceRelation& sim) override;

        explicit DenseLabelRelation(const fts::FTSTask & fts_task);

        inline int get_num_labels() const;
    };
}



#endif
