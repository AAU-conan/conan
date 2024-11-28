#ifndef DOMINANCE_LOCAL_STATE_RELATION_H
#define DOMINANCE_LOCAL_STATE_RELATION_H

#include <vector>
#include <string>
#include <iostream>
#include <memory>

namespace merge_and_shrink{
    class TransitionSystem;
}

namespace fts {
    class LabelledTransitionSystem;
}

namespace utils {
    class LogProxy;
}

namespace dominance {
    class LocalStateRelation {
    protected:
        // By now we assume that the partition is unitary... we can improve this later with EquivalenceRelation
        std::vector<std::vector<bool> > relation;

        // Vectors of states dominated/dominating by each state. Lazily computed when needed.
        std::vector<std::vector<int>> dominated_states, dominating_states;
        void compute_list_dominated_states();
    public:
        LocalStateRelation(std::vector<std::vector<bool> > && relation);

        //static LocalStateRelation get_local_distances_relation(const merge_and_shrink::TransitionSystem & ts);
        static std::unique_ptr<LocalStateRelation> get_local_distances_relation(const fts::LabelledTransitionSystem &ts);
        //TODO?: static LocalStateRelation get_identity_relation(const merge_and_shrink::TransitionSystem & ts);

        void cancel_simulation_computation();

        inline void remove(int s, int t) {
            relation[s][t] = false;
        }

        const std::vector<int> &get_dominated_states(int state);
        const std::vector<int> &get_dominating_states(int state);

        inline bool simulates(int s, int t) const {
            return !relation.empty() ? relation[s][t] : s == t;
        }

        inline bool similar(int s, int t) const {
            return !relation.empty() ?
                   relation[s][t] && relation[t][s] :
                   s == t;
        }

        inline const std::vector<std::vector<bool> > &get_relation() {
            return relation;
        }

        bool is_identity() const;
        int num_equivalences() const;
        int num_simulations(bool ignore_equivalences) const;
        int num_different_states() const;

        int num_states() const {
            return relation.size();
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
