#ifndef MERGE_AND_SHRINK_LABEL_RELATION_H
#define MERGE_AND_SHRINK_LABEL_RELATION_H

#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include "all_none_factor_index.h"
#include "factor_dominance_relation.h"

namespace fts {
    class FTSTask;
    class LabelledTransitionSystem;
    class LabelMap;
}

namespace utils {
    class LogProxy;
}

namespace dominance {

/* 
 * Label relation represents the preorder relations on labels that
 * occur in a set of LTS
 */
    class LabelRelation {
    protected:
        int num_labels;
        const fts::FTSTask& fts_task;

    public:
        explicit LabelRelation(const fts::FTSTask& fts_task);
        virtual ~LabelRelation() = default;

        [[nodiscard]] virtual bool label_dominates_label_in_all_other(int factor, int l1, int l2) const = 0;

        [[nodiscard]] virtual bool noop_simulates_label_in_all_other(int factor, int l) const = 0;

        virtual bool update_factor(int factor, const FactorDominanceRelation& sim) = 0;


        void dump(utils::LogProxy &log) const;
    };

    class LabelRelationFactory {
    public:
        virtual ~LabelRelationFactory() = default;
        virtual std::unique_ptr<LabelRelation> create(const fts::FTSTask& fts_task) = 0;
    };

    template<class LabelRelationType>
    class LabelRelationFactoryImpl final : public LabelRelationFactory {
        std::unique_ptr<LabelRelation> create(const fts::FTSTask& fts_task) override {
            return std::make_unique<LabelRelationType>(fts_task);
        }
    };


    class DenseLabelRelation : public LabelRelation {
        // Matrix factors where l1 simulates l2
        std::vector<std::vector<AllNoneFactorIndex> > dominates_in;

        //Indicates whether labels are dominated by noop or other irrelevant
        //variables in theta
        std::vector<std::vector<bool> > simulated_by_irrelevant;
        std::vector<std::vector<bool> > simulates_irrelevant;

        std::vector<AllNoneFactorIndex> dominated_by_noop_in;

        //Returns true if l1 simulates l2 in lts
        [[nodiscard]] bool simulates(int l1, int l2, int factor) const {
            assert (l1 >= 0);
            assert((size_t)l1 < dominates_in.size());
            assert (l2 >= 0);
            assert((size_t)l2 < dominates_in[l1].size());

            return dominates_in[l1][l2].contains(factor);
        }

        [[nodiscard]] bool noop_simulates(int l, int factor) const  {
            assert(l >= 0);
            assert((size_t)l < dominated_by_noop_in.size());

            return dominated_by_noop_in[l].contains(factor);
        }


        inline void set_not_simulates(int l1, int l2, int lts) {
            //std::cout << "Not simulates: " << l1 << " to " << l2 << " in " << lts << std::endl;
            dominates_in[l1][l2].remove(lts);
        }

        inline bool set_not_simulated_by_irrelevant(int l, int lts) {
            //std::cout << "Not simulated by irrelevant: " << l << " in " << lts << std::endl;

            //Returns if there were changes in dominated_by_noop_in
            simulated_by_irrelevant[l][lts] = false;
            return dominated_by_noop_in[l].remove(lts);
        }

    public:

        [[nodiscard]] bool label_dominates_label_in_all_other(int factor, int l1, int l2) const override;

        [[nodiscard]] bool noop_simulates_label_in_all_other(int factor, int l) const override;

        bool update_factor(int factor, const FactorDominanceRelation& sim) override;

        explicit DenseLabelRelation(const fts::FTSTask & fts_task);


        void dump(utils::LogProxy &log) const;
        void dump(utils::LogProxy &log, int label) const;
        void dump_equivalent(utils::LogProxy &log) const;
        void dump_dominance(utils::LogProxy &log) const;


        inline int get_num_labels() const {
            return num_labels;
        }

    };
}
    

#endif
