#ifndef QUALIFIED_LABEL_RELATION_H
#define QUALIFIED_LABEL_RELATION_H

#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <memory>

#include "../dominance/all_none_factor_index.h"


using namespace dominance;

namespace fts {
    class FTSTask;
    class LabelledTransitionSystem;
    class LabelMap;
}

namespace utils {
    class LogProxy;
}

namespace qdominance {
    class QualifiedLocalStateRelation;
    /*
     * Label relation represents the preorder relations on labels that
     * occur in a set of LTS
     */
    class QualifiedLabelRelation {
        int num_labels;
        //For each lts, matrix indicating whether l1 simulates l2
        //std::vector<std::vector<std::vector<bool > > > dominates;
        std::vector<std::vector<AllNoneFactorIndex> > dominates_in;

        //Indicates whether labels are dominated by noop or other irrelevant
        //variables in theta
        std::vector<std::vector<bool> > simulated_by_irrelevant;
        std::vector<std::vector<bool> > simulates_irrelevant;

        std::vector<AllNoneFactorIndex> dominated_by_noop_in;

        bool update(int i, const fts::LabelledTransitionSystem& lts, const QualifiedLocalStateRelation& sim);

        //Returns true if l1 simulates l2 in lts
        inline bool simulates(int l1, int l2, int lts) const {
            assert (l1 >= 0);
            assert((size_t)l1 < dominates_in.size());
            assert (l2 >= 0);
            assert((size_t)l2 < dominates_in[l1].size());

            return dominates_in[l1][l2].contains(lts);
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
        QualifiedLabelRelation(const fts::FTSTask& fts_task,
                               const std::vector<std::unique_ptr<QualifiedLocalStateRelation>>& sim);

        bool update(const fts::FTSTask& fts_task, const std::vector<std::unique_ptr<QualifiedLocalStateRelation>>& sim);

        void dump(utils::LogProxy& log) const;
        void dump(utils::LogProxy& log, int label) const;
        void dump_dominance(utils::LogProxy& log) const;


        [[nodiscard]] int get_num_labels() const {
            return num_labels;
        }

        [[nodiscard]] AllNoneFactorIndex get_dominated_by_noop_in(int l) const {
            return dominated_by_noop_in[l];
        }

        [[nodiscard]] bool dominated_by_noop(int l, int lts) const {
            return dominated_by_noop_in[l].contains_all_except(lts);
        }

        //Returns true if l dominates l2 in lts (simulates l2 in all j \neq lts)
        [[nodiscard]] bool dominates(int l1, int l2, int lts) const {
            return dominates_in[l1][l2].contains_all_except(lts);
        }

        [[nodiscard]] AllNoneFactorIndex where_dominates(int l1, int l2) const {
            return dominates_in[l1][l2];
        }
    };
}


#endif
