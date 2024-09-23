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
#include <mata/nfa/nfa.hh>
#include <mata/nfa/types.hh>
#include <boost/algorithm/string.hpp>

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
        fts::FactValueNames fact_value_names;

        // NFA representing the simulation relation
        mutable mata::nfa::Nfa simulation_nfa;

        // for s_i and s_j, the state in the NFA that represents when s_i simulates s_j
        std::vector<std::vector<mata::nfa::State>> state_pair_to_nfa_state;
        std::unordered_map<mata::nfa::State, std::pair<int,int>> nfa_state_to_state_pair;

        // The universal accepting state of simulation_nfa
        mata::nfa::State universally_accepting;

        bool nfa_always_simulates(const int s, const int t) const {
            // draw_nfa("nfa_simulates.dot");
            // std::println(std::cout, "{} <= {}", fact_value_names.get_fact_value_name(t), fact_value_names.get_fact_value_name(s));
            simulation_nfa.initial.insert(state_pair_to_nfa_state[s][t]);
            mata::nfa::Run cex;
            const bool result = simulation_nfa.is_universal(*simulation_nfa.alphabet, &cex);
            auto w = cex.word | std::views::transform([&](int i) { return fact_value_names.get_operator_name(i, false); });
            // std::println(std::cout, "{}", std::ranges::fold_left(w, std::string(), [](std::string a, std::string b) { return a + " " + b; }));
            // std::println(std::cout, "{}", result);
            simulation_nfa.initial.clear();
            return result;
        }

        bool nfa_never_simulates(const int s, const int t) const {
            simulation_nfa.initial.insert(state_pair_to_nfa_state[s][t]);
            const bool result = simulation_nfa.is_lang_empty();
            simulation_nfa.initial.clear();
            return result;
        }

        mata::nfa::State nfa_simulates(const int s, const int t) const {
            return state_pair_to_nfa_state[s][t];
        }

        int num_labels;

        // Vectors of states dominated/dominating by each state. Lazily computed when needed.
        std::vector<std::vector<int>> dominated_states, dominating_states;
        void compute_list_dominated_states();

    public:
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

        [[nodiscard]] mata::nfa::Nfa get_inverted_nfa(int s, int t) const {
            auto complement = mata::nfa::Nfa(simulation_nfa);
            complement.initial.insert(state_pair_to_nfa_state[s][t]);
            complement = determinize(complement); // These actions remove the alphabet for some reason
            complement = minimize(complement);
            complement.make_complete(simulation_nfa.alphabet);
            complement.swap_final_nonfinal();
            complement.alphabet = simulation_nfa.alphabet;
            return complement;
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
