#ifndef DOMINANCE_STATE_DOMINANCE_RELATION_H
#define DOMINANCE_STATE_DOMINANCE_RELATION_H

#include <vector>
#include <memory>

#include "factor_dominance_relation.h"

class State;

namespace utils {
    class LogProxy;
}

namespace dominance {
    class LabelRelation;

/*
 * Class that represents the collection of simulation relations for a factored task.
 * Uses unique_ptr so that it owns the local relations, and it cannot be copied away.
 */
    class StateDominanceRelation {
    protected:
        std::vector<std::unique_ptr<FactorDominanceRelation> > local_relations;
        std::unique_ptr<LabelRelation> label_relation;
    public:
        explicit StateDominanceRelation(std::vector<std::unique_ptr<FactorDominanceRelation>> &&_local_relations, std::unique_ptr<LabelRelation> &label_relation);

        //Statistics of the factored simulation
        void dump_statistics(utils::LogProxy &log) const;

        int num_equivalences() const;

        int num_simulations() const;

        double num_st_pairs() const;

        double num_states_problem() const;

        //Computes the probability of selecting a random pair s, s' such
        //that s simulates s'.
        double get_percentage_simulations(bool ignore_equivalences) const;

        //Computes the probability of selecting a random pair s, s' such that
        //s is equivalent to s'.
        double get_percentage_equivalences() const;

        double get_percentage_equal() const;

        //Methods to access the underlying simulation relations
        const std::vector<std::unique_ptr<FactorDominanceRelation> > &get_local_relations() const {
            return local_relations;
        }

        size_t size() const {
            return local_relations.size();
        }

        FactorDominanceRelation &operator[](int index) {
            return *(local_relations[index]);
        }

        const FactorDominanceRelation &operator[](int index) const {
            return *(local_relations[index]);
        }

        [[nodiscard]] const LabelRelation& get_label_relation() const {
            return *label_relation;
        }

        //Methods to use the simulation
        [[nodiscard]] bool dominates(const State &t, const State &s) const;

    // TODO (future): Integrate methods for task transformation from the fd_simulation repository

    };
}

#endif
