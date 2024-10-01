#ifndef QUALIFIED_DOMINANCE_DOMINANCE_RELATION_H
#define QUALIFIED_DOMINANCE_DOMINANCE_RELATION_H

#include <vector>
#include <memory>
#include <set>

#include "qualified_local_state_relation.h"

namespace merge_and_shrink {
    class TransitionSystem;
}
namespace utils {
    class LogProxy;
}

namespace qdominance {
/*
 * Class that represents the collection of simulation relations for a factored task.
 * Uses unique_ptr so that it owns the local relations, and it cannot be copied away.
 */
    class QualifiedFactoredDominanceRelation {
    protected:
        std::vector<std::unique_ptr<QualifiedLocalStateRelation> > local_relations;
    public:
        QualifiedFactoredDominanceRelation(std::vector<std::unique_ptr<QualifiedLocalStateRelation>> &&_local_relations) :
            local_relations (std::move(_local_relations)){
        }


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
        const std::vector<std::unique_ptr<QualifiedLocalStateRelation> > &get_local_relations() const {
            return local_relations;
        }

        size_t size() const {
            return local_relations.size();
        }

        QualifiedLocalStateRelation &operator[](int index) {
            assert(index >= 0 && index < local_relations.size());
            return *(local_relations[index]);
        }

        const QualifiedLocalStateRelation &operator[](int index) const {
            assert(index >= 0 && index < local_relations.size());
            return *(local_relations[index]);
        }

        //Methods to use the simulation
//        bool pruned_state(const State &state) const;
//        int get_cost(const State &state) const;
//        bool dominates(const State &t, const State &s) const;

    // TODO (future): Integrate methods for task transformation from the fd_simulation repository

    };
}

#endif
