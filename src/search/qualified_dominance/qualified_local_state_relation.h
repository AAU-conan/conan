#ifndef QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H
#define QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H

#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <set>
#include <unordered_set>

namespace merge_and_shrink{
    class TransitionSystem;
}

namespace fts {
    class LabelledTransitionSystem;
}

namespace utils {
    class LogProxy;
}

typedef std::set<int> QualifiedSet;

namespace qdominance {
    // First implementation of a simulation relation.
    class QualifiedLocalStateRelation {
    protected:
        // By now we assume that the partition is unitary... we can improve this later with EquivalenceRelation
        std::vector<std::vector<QualifiedSet> > relation;

        int num_labels;

        // Vectors of states dominated/dominating by each state. Lazily computed when needed.
        std::vector<std::vector<int>> dominated_states, dominating_states;
        void compute_list_dominated_states();
    public:
        QualifiedLocalStateRelation(std::vector<std::vector<QualifiedSet> > && relation, int num_labels);

        //static LocalStateRelation get_local_distances_relation(const merge_and_shrink::TransitionSystem & ts);
        static std::unique_ptr<QualifiedLocalStateRelation> get_local_distances_relation(const fts::LabelledTransitionSystem &ts, int num_labels);
        //TODO?: static LocalStateRelation get_identity_relation(const merge_and_shrink::TransitionSystem & ts);

        void cancel_simulation_computation();

        inline void remove(int s, int t, int a) {
            relation[s][t].erase(a);
        }

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
            return relation[s][t].size() == num_labels;
        }

        inline QualifiedSet labels_where_simulates(int s, int t) const {
            return relation[s][t];
        }

        inline bool similar(int s, int t) const {
            return !relation.empty() ?
                   simulates(s, t) && simulates(t, s) :
                   s == t;
        }

        inline const std::vector<std::vector<QualifiedSet> > &get_relation() {
            return relation;
        }

        bool is_identity() const;
        int num_equivalences() const;
        int num_simulations(bool ignore_equivalences) const;
        int num_different_states() const;

        int num_states() const {
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
