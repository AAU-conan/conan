#include <set>
#include <print>

#include "nfa_merge_non_differentiable.h"

#include "qualified_local_state_relation.h"
using namespace mata::nfa;

namespace qdominance {
    [[nodiscard]] Nfa determinize(const Nfa& nfa) {
        Nfa new_nfa(nfa);
        new_nfa.make_complete();

        std::unordered_map<StateSet, State> product_states;

        std::vector<State> waiting = std::views::iota(0, static_cast<int>(nfa.num_of_states())) | std::ranges::to<std::vector<State>>();

        std::function<State(const StateSet&)> get_state = [&](const StateSet& states) {
            assert(!std::ranges::any_of(states, [&](const State s) { return s >= nfa.num_of_states(); }));
            if (states.size() == 1) {
                return *states.begin();
            }
            if (!product_states.contains(states)) {
                product_states[states] = new_nfa.add_state();

                // Create transitions for new product state
                for (mata::Symbol l : nfa.alphabet->get_alphabet_symbols()) {
                    new_nfa.delta.add(product_states[states], l, get_state(nfa.post(states, l)));
                }
                if (std::ranges::any_of(states, [&](State s) { return nfa.final.contains(s); })) {
                    new_nfa.final.insert(product_states[states]);
                }

            }
            return product_states[states];
        };


        for (State s = 0; s < nfa.num_of_states(); ++s) {
            // Find all non-deterministic transitions from s
            for (SymbolPost& sp : new_nfa.delta.mutable_state_post(s)) {
                if (sp.targets.size() > 1) {
                    // Change this to only the product state
                    sp.targets = {get_state(sp.targets)};
                }
            }
        }
        return new_nfa;
    };


    [[nodiscard]] std::pair<Nfa, std::vector<State>> merge_non_differentiable_states(const Nfa& nfa, const QualifiedLocalStateRelation& relation) {
        assert(nfa.is_complete(nfa.alphabet)); // This check is not sufficient because it only checks reachable states
        Nfa detnfa(nfa);
        detnfa.make_complete();
        detnfa = qdominance::determinize(detnfa);

        // Use Hopcroft's algorithm to merge non-differentiable states
        std::set<State> all_states = std::views::iota(0, static_cast<int>(detnfa.num_of_states())) | std::ranges::to<std::set<State>>();
        std::set<State> final_states = std::set(detnfa.final.begin(), detnfa.final.end());
        std::set<State> accepting_states;
        std::ranges::set_difference(std::move(all_states), final_states, std::inserter(accepting_states, accepting_states.begin()));

        std::vector<std::set<State>> partition = {std::move(accepting_states), std::move(final_states)};
        std::set<int> waiting = {0, 1}; // Holds the indices of partition which is in waiting state

        // Maps each state s and label l, to the states that have an l-transition to s
        std::vector<std::vector<std::vector<State>>> state_label_premap = std::vector(detnfa.num_of_states(), std::vector(detnfa.alphabet->get_alphabet_symbols().size(), std::vector<State>()));
        for (auto [from, label, to] : detnfa.delta.transitions()) {
            state_label_premap[to][label].push_back(from);
        }

        while (!waiting.empty()) {
            // Pop the first element from waiting
            auto a_index = *waiting.begin();
            waiting.erase(waiting.begin());

            auto a_part = partition[a_index];
            for (mata::Symbol label : detnfa.alphabet->get_alphabet_symbols()) {
                auto a = std::views::transform(a_part, [&](const State s) { return state_label_premap[s][label]; }) | std::views::join | std::ranges::to<std::vector<State>>();
                auto states_leading_to_a = std::set<State>(a.begin(), a.end());
                for (const auto& [y_part_index, y_part] : std::views::enumerate(partition)) {
                    std::set<State> intersection;
                    std::ranges::set_intersection(states_leading_to_a, y_part, std::inserter(intersection, intersection.begin()));
                    std::set<State> difference;
                    std::ranges::set_difference(y_part, states_leading_to_a, std::inserter(difference, difference.begin()));

                    if (!intersection.empty() && !difference.empty()) {
                        // y_part is split into intersection and difference
                        partition[y_part_index] = intersection; // This invalidates y_part
                        int intersection_index = (int)y_part_index;
                        partition.push_back(difference);
                        int difference_index = (int)partition.size() - 1;

                        if (waiting.contains(intersection_index)) {
                            // Fix the waiting list
                            // intersection will now have the y_part_index, and difference will be the last element in partition
                            waiting.insert(difference_index);
                        } else {
                            // Add the smaller of the two new partitions to the waiting list
                            if (intersection.size() < difference.size()) {
                                waiting.insert(intersection_index);
                            } else {
                                waiting.insert(difference_index);
                            }
                        }
                    }
                }
            }
        }


        // Create the new NFA
        Nfa new_nfa = Nfa({}, {}, {}, detnfa.alphabet);

        // Create new states and make old to new state mapping
        std::vector<State> old_to_new(detnfa.num_of_states(), -1);
        for (const auto& [new_index, new_part] : std::views::enumerate(partition)) {
            new_nfa.add_state();

            for (State old_index : new_part) {
                old_to_new[old_index] = new_index;
            }
        }

        // Add transitions
        for (const auto& [from, label, to] : detnfa.delta.transitions()) {
            new_nfa.delta.add(old_to_new[from], label, old_to_new[to]);
        }

        // Add initial states
        for (State initial : detnfa.initial) {
            new_nfa.initial.insert(old_to_new[initial]);
        }
        assert(new_nfa.initial.size() <= 1);

        // Add final states
        for (State final : detnfa.final) {
            new_nfa.final.insert(old_to_new[final]);
        }

        return {new_nfa, old_to_new};
    }
}