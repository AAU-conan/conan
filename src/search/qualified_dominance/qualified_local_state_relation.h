#ifndef QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H
#define QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H

#include <vector>
#include <string>
#include <memory>

#include <rust/cxx.h>
#include <mata/nfa/nfa.hh>
#include <mata/nfa/types.hh>
#include <unordered_map>

#include "nfa_merge_non_differentiable.h"
#include "../factored_transition_system/fact_names.h"

namespace qdominance {
    class QualifiedLabelRelation;
}

namespace merge_and_shrink{
    class TransitionSystem;
}

namespace fts {
    class LabelledTransitionSystem;
}

namespace utils {
    class LogProxy;
}

namespace qdominance {
    // First implementation of a simulation relation.
    class QualifiedLocalStateRelation {
    protected:

        // NFA representing the simulation relation
        mutable mata::nfa::Nfa simulation_nfa;

        // Whether the simulation_nfa has been reduced. When it is reduced, we are guaranteed that the universally_accepting state
        // is the only state that is universally accepting.
        bool is_simulation_nfa_reduced = false;

        // for s_i and s_j, the state in the NFA that represents when s_i simulates s_j
        std::vector<std::vector<mata::nfa::State>> state_pair_to_nfa_state;
        std::unordered_map<mata::nfa::State, std::pair<int,int>> nfa_state_to_state_pair;

        // The universal accepting state of simulation_nfa
        mata::nfa::State universally_accepting;

        mutable std::vector<int> universally_accepting_cache;
        bool nfa_always_simulates(const int s, const int t) const {
            if (is_simulation_nfa_reduced)
                return state_pair_to_nfa_state[s][t] == universally_accepting;

            // draw_nfa("nfa_simulates.dot");
            // std::println(std::cout, "{} <= {}", fact_value_names.get_fact_value_name(t), fact_value_names.get_fact_value_name(s));
            const auto nfa_state = state_pair_to_nfa_state[s][t];
            if (universally_accepting_cache[nfa_state] != -1) {
                return universally_accepting_cache[nfa_state];
            }
            simulation_nfa.initial.insert(nfa_state);
            const bool result = simulation_nfa.is_universal(*simulation_nfa.alphabet);
            simulation_nfa.initial.clear();
            universally_accepting_cache[nfa_state] = result;
            return result;
        }

        bool nfa_never_simulates(const int s, const int t) const {
            simulation_nfa.initial.insert(state_pair_to_nfa_state[s][t]);
            const bool result = simulation_nfa.is_lang_empty();
            simulation_nfa.initial.clear();
            return result;
        }

        int num_labels;

        // Vectors of states dominated/dominating by each state. Lazily computed when needed.
        std::vector<std::vector<int>> dominated_states, dominating_states;
        void compute_list_dominated_states();
    public:
        fts::FactValueNames fact_value_names;
        QualifiedLocalStateRelation(mata::nfa::Nfa& simulation_nfa, std::vector<std::vector<mata::nfa::State> > && relation, std::unordered_map<mata::nfa::State, std::pair<int,int>>& nfa_state_to_state_pair, int num_labels, mata::nfa::State universally_accepting, fts::FactValueNames fact_value_names);

        //static LocalStateRelation get_local_distances_relation(const merge_and_shrink::TransitionSystem & ts);
        static std::unique_ptr<QualifiedLocalStateRelation> get_local_distances_relation(const fts::LabelledTransitionSystem &ts, int num_labels);
        //TODO?: static LocalStateRelation get_identity_relation(const merge_and_shrink::TransitionSystem & ts);

        void draw_nfa(const std::string& filename) const;
        void draw_transformed_nfa(const std::string& filename, const mata::nfa::Nfa& other_nfa) const;
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

        const auto& get_state_to_nfa_state() const {
            return state_pair_to_nfa_state;
        }

        bool update(int s, int t, const QualifiedLabelRelation& label_relation, const fts::LabelledTransitionSystem& ts, int lts_i, utils::
                    LogProxy& log);

        [[nodiscard]] bool simulates(int s, int t) const {
            return nfa_always_simulates(s, t);
        }

        [[nodiscard]] bool never_simulates(int s, int t) const {
            return nfa_never_simulates(s, t);
        }

        [[nodiscard]] bool similar(int s, int t) const {
            return simulates(s, t) && simulates(t, s);
        }

        mata::nfa::State nfa_simulates(const int s, const int t) const {
            return state_pair_to_nfa_state[s][t];
        }

        void reduce_nfa() {
            auto [reduced_nfa, old_to_new_state_map] = merge_non_differentiable_states(simulation_nfa, *this);
            simulation_nfa = reduced_nfa;
            universally_accepting = old_to_new_state_map[universally_accepting];
            for (int i = 0; i < state_pair_to_nfa_state.size(); ++i) {
                for (int j = 0; j < state_pair_to_nfa_state[i].size(); ++j) {
                    state_pair_to_nfa_state[i][j] = old_to_new_state_map[state_pair_to_nfa_state[i][j]];
                }
            }
        }

        bool is_identity() const;
        int num_equivalences() const;
        int num_simulations(bool ignore_equivalences) const;
        int num_different_states() const;

        [[nodiscard]] size_t num_states() const {
            return state_pair_to_nfa_state.size();
        }

        int get_num_labels() const {
            return num_labels;
        }

        [[nodiscard]] const mata::nfa::Nfa& get_nfa() const {
            return simulation_nfa;
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
