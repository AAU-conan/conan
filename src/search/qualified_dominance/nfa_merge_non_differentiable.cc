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

    // [[nodiscard]] Nfa determinize_bdd(const Nfa& nfa) {
    //     // Create a BDD manager
    //     Cudd mgr;
    //     // Create a BDD variable for each state
    //     std::vector<BDD> state_vars_1;
    //     std::vector<BDD> state_vars_2;
    //     std::vector<BDD> state_vars_3;
    //     std::vector<BDD> state_vars_4;
    //     for (State s = 0; s < nfa.num_of_states(); ++s) {
    //         state_vars_1.push_back(mgr.bddVar(s + 0 * nfa.num_of_states()));
    //         state_vars_2.push_back(mgr.bddVar(s + 1 * nfa.num_of_states()));
    //         state_vars_3.push_back(mgr.bddVar(s + 2 * nfa.num_of_states()));
    //         state_vars_4.push_back(mgr.bddVar(s + 3 * nfa.num_of_states()));
    //     }
    //
    //     BDD state_vars_1_cube = mgr.computeCube(state_vars_1);
    //     BDD state_vars_2_cube = mgr.computeCube(state_vars_2);
    //     BDD state_vars_3_cube = mgr.computeCube(state_vars_3);
    //     BDD state_vars_4_cube = mgr.computeCube(state_vars_4);
    //
    //     // Create a BDD variable for each symbol
    //     std::vector<BDD> symbol_vars;
    //     for (mata::Symbol l : nfa.alphabet->get_alphabet_symbols()) {
    //         symbol_vars.push_back(mgr.bddVar(l + 4 * nfa.num_of_states()));
    //     }
    //
    //     // Create transition relation BDD
    //     BDD transition_relation = mgr.bddZero();
    //     for (State q = 0; q < nfa.num_of_states(); ++q) {
    //         for (const auto& symbol_post : nfa.delta.state_post(q)) {
    //             BDD transition = mgr.bddOne();
    //             for (State s = 0; s < nfa.num_of_states(); ++s) {
    //                 if (s == q)
    //                     transition &= state_vars_1[s];
    //                 else
    //                     transition &= !state_vars_1[s];
    //
    //                 if (symbol_post.targets.contains(s))
    //                     transition &= state_vars_2[s];
    //                 else
    //                     transition &= !state_vars_2[s];
    //             }
    //
    //             for (const mata::Symbol& l : nfa.alphabet->get_alphabet_symbols()) {
    //                 if (l == symbol_post.symbol)
    //                     transition &= symbol_vars[l];
    //                 else
    //                     transition &= !symbol_vars[l];
    //             }
    //             transition_relation |= transition;
    //         }
    //     }
    //
    //     BDD powerset_relation = transition_relation;
    //
    //     while (true) {
    //         BDD powerset_relation_2 = powerset_relation.SwapVariables(state_vars_2, state_vars_3);
    //         BDD union_2_3 = (state_vars_4_cube & (state_vars_2_cube | state_vars_3_cube)) | (!state_vars_4_cube & !(state_vars_2_cube | state_vars_3_cube));
    //         powerset_relation = !(powerset_relation & powerset_relation_2) | union_2_3;
    //     }
    //
    //     // Create a BDD that represents the partitioning of the states, one with all final states and one with all non-final states
    //     BDD final_partition = mgr.bddOne();
    //     BDD non_final_partition = mgr.bddOne();
    //     for (State s = 0; s < nfa.num_of_states(); ++s) {
    //         if (nfa.final.contains(s)) {
    //             final_partition &= state_vars_1[s];
    //         } else {
    //             non_final_partition &= state_vars_1[s];
    //         }
    //     }
    //
    //     BDD partition = final_partition | non_final_partition;
    //
    // }

    [[nodiscard]] Nfa determinize2(const Nfa& aut) {
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
    }


    [[nodiscard]] Nfa revert2(const Nfa& aut) {
        Nfa result;
        result.clear();

        const size_t num_of_states{ aut.num_of_states() };
        result.delta.allocate(num_of_states);

        for (State sourceState{ 0 }; sourceState < num_of_states; ++sourceState) {
            for (const SymbolPost &transition: aut.delta[sourceState]) {
                for (const State targetState: transition.targets) {
                    result.delta.add(targetState, transition.symbol, sourceState);
                }
            }
        }

        result.initial = aut.final;
        result.final = aut.initial;

        return result;

    }

    [[nodiscard]] Nfa minimize2(const Nfa& nfa) {
        return determinize2(revert2(determinize2(revert2(nfa))));
    }


    [[nodiscard]] std::pair<Nfa, std::vector<State>> merge_non_differentiable_states(const Nfa& nfa, const QualifiedLocalStateRelation& relation) {
        assert(nfa.is_complete(nfa.alphabet)); // This check is not sufficient because it only checks reachable states
        std::println("Pre determinization size {}", nfa.num_of_states());
        Nfa detnfa(nfa);
        detnfa.make_complete();
        detnfa = qdominance::determinize2(detnfa);
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

    // Minimize NFA by combining minimization and merging non-differentiable states steps
    [[nodiscard]] std::pair<mata::nfa::Nfa, std::vector<State>> minimize3(const mata::nfa::Nfa& nfa) {
        utils::g_log << "Final constraint" << std::endl;
        // Compute a simulation relation over the states of the NFA
        // for q, and p, whether q simulates p, i.e. p <= q
        std::vector<std::vector<bool>> simulates(nfa.num_of_states(), std::vector<bool>(nfa.num_of_states(), true));
        // First, apply the final state constraint: q <= p => (q ∈ F => p ∈ F)
        for (State q = 0; q < nfa.num_of_states(); ++q) {
            for (State p = 0; p < nfa.num_of_states(); ++p) {
                if (nfa.final.contains(q) && !nfa.final.contains(p)) {
                    simulates[q][p] = false;
                }
            }
        }


        utils::g_log << "Transition constraint" << std::endl;
        // Now apply the transition constraint, i.e. q <= p => (q --l-> q' => (p --l-> p' and q' <= p'))
        bool change = false;
        do {
            change = false;
            for (State q = 0; q < nfa.num_of_states(); ++q) {
                for (State p = 0; p < nfa.num_of_states(); ++p) {
                    if (!simulates[q][p]) {
                        continue;
                    }
                    for (const SymbolPost& q_post : nfa.delta[q]) {
                        for (State q_target : q_post.targets) {
                            bool found = false;
                            for (const SymbolPost& p_post : nfa.delta[p]) {
                                if (p_post.symbol == q_post.symbol) {
                                    for (State p_target : p_post.targets) {
                                        if (simulates[q_target][p_target]) {
                                            found = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (!found) {
                                simulates[q][p] = false;
                                change = true;
                                break;
                            }
                        }
                    }
                }
            }
        } while(change);

        auto new_nfa = Nfa();
        new_nfa.alphabet = nfa.alphabet;

        utils::g_log << "Merge bisimilar states" << std::endl;
        // Merge bisimilar states, i.e. merge p,q if p <= q and q <= p
        std::vector<State> old_to_new(nfa.num_of_states(), -1);
        std::vector<State> new_to_old;
        // Create new states for the combined states
        for (State q = 0; q < nfa.num_of_states(); ++q) {
            if (old_to_new[q] != -1) {
                continue;
            }
            old_to_new[q] = new_nfa.add_state();
            new_to_old.push_back(q);
            if (nfa.final.contains(q)) {
                new_nfa.final.insert(old_to_new[q]);
            }
            if (nfa.initial.contains(q)) {
                new_nfa.initial.insert(old_to_new[q]);
            }
            for (State p = q + 1; p < nfa.num_of_states(); ++p) {
                if (simulates[q][p] && simulates[p][q]) {
                    old_to_new[p] = old_to_new[q];
                }
            }
        }

        utils::g_log << "Simplify transitions" << std::endl;
        // Now apply simplification. Whenever q --l-> q' and q --l--> q'' where q' <= q'', then remove q --l-> q'
        for (State q = 0; q < nfa.num_of_states(); ++q) {
            for (const SymbolPost& q_post : nfa.delta.state_post(q)) {
                for (State q_target : q_post.targets) {
                    bool is_simulated = false;
                    for (State q_target2 : q_post.targets) {
                        if (simulates[q_target2][q_target] && (!simulates[q_target][q_target2] || q_target < q_target2)) {
                            is_simulated = true;
                            break;
                        }
                    }
                    if (!is_simulated) {
                        new_nfa.delta.add(q, q_post.symbol, q_target);
                    }
                }
            }
        }

        utils::g_log << "Pre determinization size " << new_nfa.num_of_states() << std::endl;
        auto res = qdominance::determinize(new_nfa);
        utils::g_log << "Post determinization size " << res.num_of_states() << std::endl;
        return {res, old_to_new};
    }

    [[nodiscard]] Nfa intersection(const Nfa& lhs, const Nfa& rhs) {
        Nfa res;
        std::vector<std::pair<State, State>> todo;


    }
}