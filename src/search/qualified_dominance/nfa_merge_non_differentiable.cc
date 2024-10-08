#include <set>
#include <print>
#include <boost/dynamic_bitset.hpp>

#include "nfa_merge_non_differentiable.h"

#include <boost/lexical_cast/detail/converter_numeric.hpp>

#include "qualified_local_state_relation.h"
#include "../algorithms/dynamic_bitset.h"


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
        std::println("Pre determinization size {}", nfa.num_of_states());
        Nfa detnfa(nfa);
        detnfa.make_complete();
        detnfa = qdominance::determinize(detnfa);
        std::println("Post determinization size {}", detnfa.num_of_states());

        // Use Hopcroft's algorithm to merge non-differentiable states
        boost::dynamic_bitset final_states = boost::dynamic_bitset(detnfa.num_of_states(), false);
        for (State s : detnfa.final) {
            final_states.set(s, true);
        }
        boost::dynamic_bitset<> non_final_states = ~final_states;

        std::vector<boost::dynamic_bitset<>> partition = {final_states, non_final_states};
        std::set<int> waiting = {0, 1}; // Holds the indices of partition which is in waiting state

        // Maps each state s and label l, to the states that have an l-transition to s
        std::vector<std::vector<boost::dynamic_bitset<>>> state_label_premap = std::vector(detnfa.num_of_states(), std::vector(detnfa.alphabet->get_alphabet_symbols().size(), boost::dynamic_bitset<>(detnfa.num_of_states(), false)));
        for (auto [from, label, to] : detnfa.delta.transitions()) {
            state_label_premap[to][label].set(from);
        }

        while (!waiting.empty()) {
            // Pop the first element from waiting
            auto a_index = *waiting.begin();
            waiting.erase(waiting.begin());

            auto a_part = partition[a_index];
            for (mata::Symbol label : detnfa.alphabet->get_alphabet_symbols()) {
                auto states_leading_to_a = *std::ranges::fold_left_first(std::views::iota(0ul, detnfa.num_of_states())
                       | std::views::filter([&](const State s) {return a_part.test(s);})
                       | std::views::transform([&](const State s) { return state_label_premap[s][label]; }),
                       [](const auto& a, const auto& b) { return a | b; });

                for (int y_part_index = 0; y_part_index < partition.size(); ++y_part_index) {
                    boost::dynamic_bitset<> difference = partition[y_part_index] - states_leading_to_a;
                    boost::dynamic_bitset<> intersection = partition[y_part_index] & states_leading_to_a;

                    if (!intersection.none() && !difference.none()) {
                        // y_part is split into intersection and difference
                        partition[y_part_index] = std::move(intersection); // This invalidates y_part
                        int intersection_index = (int)y_part_index;
                        partition.push_back(std::move(difference));
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

            for (State old_index = 0; old_index < detnfa.num_of_states(); ++old_index) {
                if (new_part.test(old_index)) {
                    old_to_new[old_index] = new_index;
                }
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

        std::println("Post merging step {}", new_nfa.num_of_states());
        return {new_nfa, old_to_new};
    }
}