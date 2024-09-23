#include "qualified_local_state_relation.h"

#include "../factored_transition_system/labelled_transition_system.h"
#include "../merge_and_shrink/transition_system.h"
#include "../utils/logging.h"
#include "qualified_label_relation.h"
#include "../utils/graphviz.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <ext/slist>
#include <utility>

#include <mata/nfa/nfa.hh>


using namespace std;
using merge_and_shrink::TransitionSystem;
using fts::LabelledTransitionSystem;

namespace qdominance {

    unique_ptr<QualifiedLocalStateRelation> QualifiedLocalStateRelation::get_local_distances_relation(const LabelledTransitionSystem & ts, int num_labels) {
        // Create list of all labels 0, 1, ..., num_labels - 1
        auto all_labels = views::iota(0, num_labels) | ranges::to<vector>();

        mata::nfa::Nfa simulation_nfa({}, {}, {}, new mata::EnumAlphabet(all_labels.begin(), all_labels.end()));
        std::vector<std::vector<mata::nfa::State> > state_pair_to_nfa_state;
        std::unordered_map<mata::nfa::State, std::pair<int,int>> nfa_state_to_state_pair;

        // Create a state that is universally accepting
        mata::nfa::State universally_accepting = simulation_nfa.add_state();
        simulation_nfa.final.insert(universally_accepting);
        for (auto l : all_labels) {
            simulation_nfa.delta.add(universally_accepting, l, universally_accepting);
        }

        // Initialize the state map and NFA states
        int num_states = ts.size();
        const std::vector<bool> &goal_states = ts.get_goal_states();
        state_pair_to_nfa_state.resize(num_states);
        for (int i = 0; i < num_states; i++) {
            state_pair_to_nfa_state[i].resize(num_states);
            for (int j = 0; j < num_states; j++) {
                mata::nfa::State nfa_state = simulation_nfa.add_state();
                state_pair_to_nfa_state[i][j] = nfa_state;
                nfa_state_to_state_pair[nfa_state] = std::make_pair(i, j);
                if (goal_states[i] || !goal_states[j]) {
                    // Set accepting; it fulfills the goal condition
                    simulation_nfa.final.insert(state_pair_to_nfa_state[i][j]);
                }
            }
        }

        // Initialize NFA transitions
        for (int i = 0; i < num_states; i++) {
            for (int j = 0; j < num_states; j++) {
                if (goal_states[i] || !goal_states[j]) {
                    // If s_i and s_j fulfill the goal condition, add simulation responses. Otherwise, add no transitions, so the state accepts nothing.
                    std::set labels_j_does_not_have(all_labels.begin(), all_labels.end());

                    // For each transition that s_j --l_j--> s_j'
                    for (const auto& t_j : ts.get_transitions(j)) {
                        for (const auto& l_j : ts.get_labels(t_j.label_group)) {
                            labels_j_does_not_have.erase(l_j);

                            // When s_i --l_j--> s_i'
                            // Add a noop edge from (s_i, s_j) --l_j--> (s_i', s_j)
                            simulation_nfa.delta.add(state_pair_to_nfa_state[i][j], l_j, state_pair_to_nfa_state[i][t_j.target]);

                            // And add edge from (s_i, s_j) --l_j--> (s_i', s_j')
                            for (const auto& t_i : ts.get_transitions(i)) {
                                simulation_nfa.delta.add(state_pair_to_nfa_state[i][j], l_j, state_pair_to_nfa_state[t_i.target][t_j.target]);
                            }
                        }
                    }
                    // For each label l_j, s.t. s_j has no l_j transition, add an edge to universal acceptance state
                    for (const auto& l_j : labels_j_does_not_have) {
                        simulation_nfa.delta.add(state_pair_to_nfa_state[i][j], l_j, universally_accepting);
                    }
                }
            }
        }

        return make_unique<QualifiedLocalStateRelation> (simulation_nfa, std::move(state_pair_to_nfa_state), nfa_state_to_state_pair, num_labels, universally_accepting, ts.fact_value_names);
    }

    int QualifiedLocalStateRelation::num_equivalences() const {
        int num = 0;
        std::vector<bool> counted(state_pair_to_nfa_state.size(), false);
        for (size_t i = 0; i < counted.size(); i++) {
            if (!counted[i]) {
                for (size_t j = i + 1; j < state_pair_to_nfa_state.size(); j++) {
                    if (similar(i, j)) {
                        counted[j] = true;
                    }
                }
            } else {
                num++;
            }
        }
        return num;
    }

    int QualifiedLocalStateRelation::num_simulations(bool ignore_equivalences) const {
        int res = 0;
        if (ignore_equivalences) {
            std::vector<bool> counted(state_pair_to_nfa_state.size(), false);
            for (size_t i = 0; i < state_pair_to_nfa_state.size(); ++i) {
                if (!counted[i]) {
                    for (size_t j = i + 1; j < state_pair_to_nfa_state.size(); ++j) {
                        if (similar(i, j)) {
                            counted[j] = true;
                        }
                    }
                }
            }
            for (size_t i = 0; i < state_pair_to_nfa_state.size(); ++i) {
                if (!counted[i]) {
                    for (size_t j = i + 1; j < state_pair_to_nfa_state.size(); ++j) {
                        if (!counted[j]) {
                            if (!similar(i, j) && (simulates(i, j) || simulates(j, i))) {
                                res++;
                            }
                        }
                    }
                }
            }
        } else {
            for (size_t i = 0; i < state_pair_to_nfa_state.size(); ++i)
                for (size_t j = 0; j < state_pair_to_nfa_state.size(); ++j)
                    if (simulates(i, j))
                        res++;
        }
        return res;
    }

    int QualifiedLocalStateRelation::num_different_states() const {
        int num = 0;
        std::vector<bool> counted(state_pair_to_nfa_state.size(), false);
        for (size_t i = 0; i < counted.size(); i++) {
            if (!counted[i]) {
                num++;
                for (size_t j = i + 1; j < state_pair_to_nfa_state.size(); j++) {
                    if (similar(i, j)) {
                        counted[j] = true;
                    }
                }
            }
        }
        return num;
    }

//Computes the probability of selecting a random pair s, s' such
//that s simulates s'.
    double QualifiedLocalStateRelation::get_percentage_simulations(bool ignore_equivalences) const {
        double num_sims = num_simulations(ignore_equivalences);
        double num_states = (ignore_equivalences ? num_different_states() : state_pair_to_nfa_state.size());
        return num_sims / (num_states * num_states);
    }

//Computes the probability of selecting a random pair s, s' such that
//s is equivalent to s'.
    double QualifiedLocalStateRelation::get_percentage_equivalences() const {
        double num_eq = 0;
        double num_states = state_pair_to_nfa_state.size();
        for (size_t i = 0; i < state_pair_to_nfa_state.size(); ++i)
            for (size_t j = 0; j < state_pair_to_nfa_state.size(); ++j)
                if (similar(i, j))
                    num_eq++;
        return num_eq / (num_states * num_states);
    }

    void QualifiedLocalStateRelation::compute_list_dominated_states() {
        dominated_states.resize(state_pair_to_nfa_state.size());
        dominating_states.resize(state_pair_to_nfa_state.size());

        for (size_t s = 0; s < state_pair_to_nfa_state.size(); ++s) {
            for (size_t t = 0; t < state_pair_to_nfa_state.size(); ++t) {
                if (simulates(t, s)) {
                    dominated_states[t].push_back(s);
                    dominating_states[s].push_back(t);
                }
            }
        }
    }

    void QualifiedLocalStateRelation::draw_nfa(const std::string &filename) const {
        graphviz::Graph g(true);
        g.add_node("true", simulation_nfa.final.contains(universally_accepting)? "shape=doublecircle": "");
        for (size_t i = 1; i < simulation_nfa.num_of_states(); ++i) {
            auto [s, t] = nfa_state_to_state_pair.at(i);
            g.add_node(std::format("{} <= {}", fact_value_names.get_fact_value_name(t), fact_value_names.get_fact_value_name(s)), simulation_nfa.final.contains(i)? "peripheries=2": "");
        }
        for (auto const& [from, label, to] : simulation_nfa.delta.transitions()) {
            g.add_edge(from, to, fact_value_names.get_operator_name(label, false));
        }
        g.output_graph(filename);
    }

    void QualifiedLocalStateRelation::draw_transformed_nfa(const std::string &filename, const mata::nfa::Nfa& other_nfa) const {
        graphviz::Graph g(true);
        for (size_t i = 0; i < other_nfa.num_of_states(); ++i) {
            g.add_node(std::format("{}", i), other_nfa.final.contains(i)? "peripheries=2": "");
        }
        for (auto const& [from, label, to] : other_nfa.delta.transitions()) {
            g.add_edge(from, to, fact_value_names.get_operator_name(label, false));
        }
        for (auto initial_state : other_nfa.initial) {
            auto invisible_node = g.add_node("", "shape=point");
            g.add_edge(invisible_node, initial_state, "");
        }
        g.output_graph(filename);
    }

    void QualifiedLocalStateRelation::cancel_simulation_computation() {
        vector<vector<mata::nfa::State> >().swap(state_pair_to_nfa_state);
    }

    // Update the simulation relation for when t simulates s
    bool QualifiedLocalStateRelation::update(int s, int t, const QualifiedLabelRelation& label_relation, const LabelledTransitionSystem& ts, int lts_i, utils::LogProxy & log) {
        std::vector<mata::nfa::Transition> to_remove;
        for (const auto& [label, target] : simulation_nfa.delta[nfa_simulates(t, s)].moves()) {
            if (target == universally_accepting)
                continue;

            auto [tp, sp] = nfa_state_to_state_pair.at(target);
            log << "    (" << ts.state_name(t) << ", " << ts.state_name(s) << ") has an edge to (" << ts.state_name(tp) << ", " << ts.state_name(sp) << ") with label " << ts.label_name(label) << std::endl;

            if (t == tp && label_relation.dominated_by_noop(int(label), lts_i)) {
                log << "        " << "which is dominated by noop" << std::endl;
                continue;
            }

            bool found = ts.applyPostSrc(t, [&](const auto& trt) {
                if (trt.target == tp) {
                    for (const auto& l : ts.get_labels(trt.label_group)) {
                        log << "        " << ts.state_name(t) << " has transition to " << ts.state_name(tp) << " with label " << ts.label_name(l);
                        if (label_relation.dominates(l, int(label), lts_i)) {
                            log << " which dominates" << std::endl;
                            return true;
                        }
                        log << " which does not dominate" << std::endl;
                    }
                }
                return false;
            });

            if (!found) {
                to_remove.emplace_back(nfa_simulates(t, s), label, target);
            }
        }

        for (const auto& [from, label, to] : to_remove) {
            simulation_nfa.delta.remove(from, label, to);
        }
        return !to_remove.empty();
    }

    QualifiedLocalStateRelation::QualifiedLocalStateRelation(mata::nfa::Nfa& simulation_nfa, vector <std::vector<mata::nfa::State>> && state_pair_to_nfa_state, std::unordered_map<mata::nfa::State, std::pair<int,int>>& nfa_state_to_state_pair, int num_labels, mata::nfa::State universally_accepting, fts::FactValueNames fact_value_names) :
        simulation_nfa(std::move(simulation_nfa)), state_pair_to_nfa_state(std::move(state_pair_to_nfa_state)), nfa_state_to_state_pair(std::move(nfa_state_to_state_pair)), universally_accepting(universally_accepting), num_labels(num_labels), fact_value_names(fact_value_names) {

    }

    /*
    bool LocalStateRelation::simulates(const State &t, const State &s) const {
        int tid = abs->get_abstract_state(t);
        int sid = abs->get_abstract_state(s);
        return relation[tid][sid];
    }


    bool LocalStateRelation::pruned(const State &state) const {
        return abs->get_abstract_state(state) == -1;
    }

    int LocalStateRelation::get_cost(const State &state) const {
        return abs->get_cost(state);
    }

    int LocalStateRelation::get_index(const State &state) const {
        return abs->get_abstract_state(state);
    }

    const std::vector<int> &LocalStateRelation::get_dominated_states(const State &state) {
        if (dominated_states.empty()) {
            compute_list_dominated_states();
        }
        return dominated_states[abs->get_abstract_state(state)];
    }

    const std::vector<int> &LocalStateRelation::get_dominating_states(const State &state) {
        if (dominated_states.empty()) {
            compute_list_dominated_states();
        }
        return dominating_states[abs->get_abstract_state(state)];
    }
*/

}
