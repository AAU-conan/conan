#ifndef MERGE_AND_SHRINK_LABEL_RELATION_H
#define MERGE_AND_SHRINK_LABEL_RELATION_H

#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include "../dominance/all_none_factor_index.h"
#include "qualified_local_state_relation.h"

#include <spot/twaalgos/contains.hh>

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
    /*
     * Label relation represents the preorder relations on labels that
     * occur in a set of LTS
     */
    class QualifiedLabelRelation {
        int num_labels;
        //For each lts, matrix indicating whether l1 simulates l2
        //std::vector<std::vector<std::vector<bool > > > dominates;

        // For label l1, l2 and lts_i, l2 dominates l1 in lts_i under the formula
        std::vector<std::vector<std::vector<QualifiedFormula>>> dominates_in;

        // For l1 and lts_i, noop dominates l1 in lts_i under the formula
        std::vector<std::vector<QualifiedFormula>> dominated_by_noop_in;

        bool update(int i, const fts::LabelledTransitionSystem& lts, const QualifiedLocalStateRelation& sim);

        //Returns true if l1 simulates l2 in lts
        [[nodiscard]] QualifiedFormula qualified_simulates(int l1, int l2, int lts) const {
            assert(l1 >= 0);
            assert((size_t)l1 < dominates_in.size());
            assert(l2 >= 0);
            assert((size_t)l2 < dominates_in[l1].size());

            return dominates_in[l1][l2][lts];
        }

        bool set_dominates_in(int l1, int l2, int lts, const QualifiedFormula& f) {
            assert(l1 >= 0);
            assert((size_t)l1 < dominates_in.size());
            assert(l2 >= 0);
            assert((size_t)l2 < dominates_in[l1].size());
            assert(lts >= 0);
            assert((size_t)lts < dominates_in[l1][l2].size());

            if (spot::are_equivalent(dominates_in[l1][l2][lts], f))
                return false;
            dominates_in[l1][l2][lts] = f;
            return true;
        }

        bool set_dominated_by_noop_in(int l, int lts, const QualifiedFormula& f) {
            assert(l >= 0);
            assert((size_t)l < dominated_by_noop_in.size());
            assert(lts >= 0);
            assert((size_t)lts < dominated_by_noop_in[l].size());

            if (spot::are_equivalent(dominated_by_noop_in[l][lts] , f))
                return false;

            dominated_by_noop_in[l][lts] = f;
            return true;
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


        [[nodiscard]] QualifiedFormula dominated_by_noop(int l, int lts) const {
            std::vector<QualifiedFormula> dominated_by_noop_not_lts;
            for (int i = 0; i < int(dominated_by_noop_in[l].size()); ++i) {
                if (i != lts) {
                    dominated_by_noop_not_lts.push_back(dominated_by_noop_in[l][i]);
                }
            }
            return spot::are_equivalent(QualifiedFormula::And(dominated_by_noop_not_lts), QualifiedFormula::tt()) ?
                   QualifiedFormula::tt() :
                   QualifiedFormula::ff();
        }

        [[nodiscard]] QualifiedFormula dominates(int l1, int l2, int lts) const {
            std::vector<QualifiedFormula> dominates_not_lts;
            for (int i = 0; i < int(dominates_in[l1][l2].size()); ++i) {
                if (i != lts) {
                    dominates_not_lts.push_back(dominates_in[l1][l2][i]);
                }
            }
            return spot::are_equivalent(QualifiedFormula::And(dominates_not_lts), QualifiedFormula::tt()) ?
                   QualifiedFormula::tt() :
                   QualifiedFormula::ff();
        }
    };
}


#endif
