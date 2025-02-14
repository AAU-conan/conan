#ifndef DOMINANCE_DENSE_FACTOR_RELATION_H
#define DOMINANCE_DENSE_FACTOR_RELATION_H

#include <vector>

#include "factor_dominance_relation.h"

namespace dominance {
    /**
     * DenseLocalStateRelation represents the simulation relation between states in a single LTS. It is implemented as a
     * dense matrix. An N x N matrix for the N states in the LTS, representing when one state simulates another.
     */
    class DenseFactorRelation final : public FactorDominanceRelation {
    protected:
        // Relations between states. relation[s][t] is true if s simulates t.
        std::vector<std::vector<bool> > relation;

    public:
        explicit DenseFactorRelation(const fts::LabelledTransitionSystem& lts);

        void remove(int s, int t) {
            relation[s][t] = false;
        }

        [[nodiscard]] bool simulates(int s, int t) const override {
            return !relation.empty() ? relation[s][t] : s == t;
        }

        [[nodiscard]] bool similar(int s, int t) const override {
            return !relation.empty() ?
                   relation[s][t] && relation[t][s] :
                   s == t;
        }

        inline const std::vector<std::vector<bool> > &get_relation() {
            return relation;
        }

        bool apply_to_simulations_until(std::function<bool(int s, int t)> &&f) const override;
        bool remove_simulations_if(std::function<bool(int s, int t)> &&f) override;
    };
}

#endif
