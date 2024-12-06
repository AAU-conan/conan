#include <set>
#include <print>
#include <boost/dynamic_bitset.hpp>

#include "nfa_merge_non_differentiable.h"

#include <boost/lexical_cast/detail/converter_numeric.hpp>

#include "qualified_local_state_relation.h"
#include "../algorithms/dynamic_bitset.h"
#include "../utils/logging.h"



using namespace mata::nfa;

namespace qdominance {
    [[nodiscard]] Nfa determinize(const Nfa& aut) {
        Nfa result{};
        result.alphabet = aut.alphabet;
        //assuming all sets targets are non-empty
        std::vector<std::pair<State, StateSet>> worklist{};
        std::unordered_map<StateSet, State> subset_map_local{};
        auto subset_map = &subset_map_local;

        for (mata::nfa::State s{0}; s < aut.num_of_states(); ++s) {
            const StateSet S{ s };
            const State Sid{ result.add_state() };

            if (aut.final.intersects_with(S)) {
                result.final.insert(Sid);
            }
            worklist.emplace_back(Sid, S);
            (*subset_map)[mata::utils::OrdVector<State>(S)] = Sid;
        }
        if (aut.delta.empty()) { return result; }

        using Iterator = mata::utils::OrdVector<SymbolPost>::const_iterator;
        SynchronizedExistentialSymbolPostIterator synchronized_iterator;

        while (!worklist.empty()) {
            const auto Spair = worklist.back();
            worklist.pop_back();
            const StateSet S = Spair.second;
            const State Sid = Spair.first;
            if (S.empty()) {
                // This should not happen assuming all sets targets are non-empty.
                break;
            }

            // add moves of S to the sync ex iterator
            synchronized_iterator.reset();
            for (State q: S) {
                mata::utils::push_back(synchronized_iterator, aut.delta[q]);
            }

            while (synchronized_iterator.advance()) {
                // extract post from the synchronized_iterator iterator
                const std::vector<Iterator>& symbol_posts = synchronized_iterator.get_current();
                mata::Symbol currentSymbol = (*symbol_posts.begin())->symbol;
                StateSet T = synchronized_iterator.unify_targets();

                const auto existingTitr = subset_map->find(T);
                State Tid;
                if (existingTitr != subset_map->end()) {
                    Tid = existingTitr->second;
                } else {
                    Tid = result.add_state();
                    (*subset_map)[mata::utils::OrdVector<State>(T)] = Tid;
                    if (aut.final.intersects_with(T)) {
                        result.final.insert(Tid);
                    }
                    worklist.emplace_back(Tid, T);
                }
                result.delta.mutable_state_post(Sid).insert(SymbolPost(currentSymbol, Tid));
            }
        }
        return result;
    }

    [[nodiscard]] std::pair<Nfa, std::vector<State>> merge_non_differentiable_states(const Nfa& nfa) {
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