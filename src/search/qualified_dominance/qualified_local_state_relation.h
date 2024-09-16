#ifndef QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H
#define QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H

#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <set>
#include <unordered_set>

#include <rust/cxx.h>
#include <spot/tl/formula.hh>
#include <spot/tl/print.hh>
#include <spot/twaalgos/contains.hh>

namespace merge_and_shrink{
    class TransitionSystem;
}

namespace fts {
    class LabelledTransitionSystem;
}

namespace utils {
    class LogProxy;
}

typedef spot::formula QualifiedFormula;

namespace qdominance {
    // First implementation of a simulation relation.
    class QualifiedLocalStateRelation {
    protected:
        // By now we assume that the partition is unitary... we can improve this later with EquivalenceRelation
        std::vector<std::vector<QualifiedFormula> > relation;

        int num_labels;

        // Vectors of states dominated/dominating by each state. Lazily computed when needed.
        std::vector<std::vector<int>> dominated_states, dominating_states;
        void compute_list_dominated_states();
    public:
        QualifiedLocalStateRelation(std::vector<std::vector<QualifiedFormula> > && relation, int num_labels);

        //static LocalStateRelation get_local_distances_relation(const merge_and_shrink::TransitionSystem & ts);
        static std::unique_ptr<QualifiedLocalStateRelation> get_local_distances_relation(const fts::LabelledTransitionSystem &ts, int num_labels);
        //TODO?: static LocalStateRelation get_identity_relation(const merge_and_shrink::TransitionSystem & ts);

        void cancel_simulation_computation();

        // This should be part of the factored mapping
        // bool pruned(const State &state) const;
        // int get_cost(const State &state) const;
        // int get_index(const State &state) const;
        //const std::vector<int> &get_dominated_states(const State &state);
        //const std::vector<int> &get_dominating_states(const State &state);
        // bool simulates(const State &t, const State &s) const;

        const std::vector<int> &get_dominated_states(int state);
        const std::vector<int> &get_dominating_states(int state);

        inline bool simulates(int s, int t) const {
            return spot::are_equivalent(relation[s][t], spot::formula::tt());
        }

        inline QualifiedFormula simulates_under(int s, int t) const {
            return relation[s][t];
        }

        bool set_simulates_under(int s, int t, const QualifiedFormula& f) {
            if (spot::are_equivalent(relation[s][t], f))
                return false;
            relation[s][t] = f;
            return true;
        }

        inline bool similar(int s, int t) const {
            return !relation.empty() ?
                   simulates(s, t) && simulates(t, s) :
                   s == t;
        }

        inline const std::vector<std::vector<QualifiedFormula> > &get_relation() {
            return relation;
        }

        bool is_identity() const;
        int num_equivalences() const;
        int num_simulations(bool ignore_equivalences) const;
        int num_different_states() const;

        [[nodiscard]] size_t num_states() const {
            return relation.size();
        }

        int get_num_labels() const {
            return num_labels;
        }

        void dump(utils::LogProxy &log, const std::vector<std::string> &names) const;
        void dump(utils::LogProxy &log) const;


        //Computes the probability of selecting a random pair s, s' such that s simulates s'.
        double get_percentage_simulations(bool ignore_equivalences) const;

        //Computes the probability of selecting a random pair s, s' such that s is equivalent to s'.
        double get_percentage_equivalences() const;

    };
}
#endif
