#ifndef QUALIFIED_DOMINANCE_DOMINANCE_RELATION_H
#define QUALIFIED_DOMINANCE_DOMINANCE_RELATION_H

#include <utility>
#include <vector>
#include <memory>
#include <set>

#include "qualified_local_state_relation2.h"

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
        std::vector<std::unique_ptr<QualifiedLocalStateRelation2> > local_relations;
        QualifiedLabelRelation label_relation;
    public:
        explicit QualifiedFactoredDominanceRelation(std::vector<std::unique_ptr<QualifiedLocalStateRelation2>> &&_local_relations, QualifiedLabelRelation  _label_relation)
                : local_relations (std::move(_local_relations)), label_relation(std::move(_label_relation)){
        }


        void print_simulations() {
            for (const auto &[factor, sim] : std::views::enumerate(local_relations)) {
                std::println("Factor {}", factor);
                sim->print_simulations();
            }
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
        const std::vector<std::unique_ptr<QualifiedLocalStateRelation2> > &get_local_relations() const {
            return local_relations;
        }

        [[nodiscard]] const QualifiedLabelRelation& get_label_relation() const {
            return label_relation;
        }

        size_t size() const {
            return local_relations.size();
        }

        QualifiedLocalStateRelation2 &operator[](int index) {
            assert(index >= 0 && index < local_relations.size());
            return *(local_relations[index]);
        }

        const QualifiedLocalStateRelation2 &operator[](int index) const {
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
