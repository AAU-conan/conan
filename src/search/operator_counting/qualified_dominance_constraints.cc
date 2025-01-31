#include "qualified_dominance_constraints.h"

#include "../qualified_dominance/nfa_merge_non_differentiable.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../factored_transition_system/draw_fts.h"
#include "../plugins/plugin.h"

#include <print>

#include <boost/unordered_map.hpp>

#include "delete_relaxation_if_constraints.h"
#include "../utils/graphviz.h"

namespace operator_counting {
    std::pair<mata::nfa::Nfa,std::vector<std::vector<mata::nfa::State>>> construct_transition_response_nfa(int factor, const LabelledTransitionSystem& lts, const qdominance::QualifiedLocalStateRelation& rel, const qdominance::QualifiedLabelRelation& label_relation, bool under_approximate = false);
    void draw_nfa(const std::string& file_name, const mata::nfa::Nfa& nfa, const fts::LabelledTransitionSystem& lts, const std::vector<std::vector<unsigned long>>& state_pair_to_nfa_state);


    [[nodiscard]] std::pair<mata::nfa::Nfa,std::vector<std::vector<mata::nfa::State>>> construct_transition_response_nfa(const int factor, const LabelledTransitionSystem& lts, const qdominance::QualifiedLocalStateRelation& rel, const qdominance::QualifiedLabelRelation& label_relation, bool under_approximate) {
        mata::nfa::Nfa nfa;
        auto all_labels = std::views::iota(0, lts.get_num_labels()) | std::ranges::to<std::vector>();
        nfa.alphabet = new mata::EnumAlphabet(all_labels.begin(), all_labels.end());
        std::vector<std::vector<mata::nfa::State>> state_pair_to_nfa_state(rel.num_states(), std::vector<mata::nfa::State>(rel.num_states()));
        // We know that all states where t simulates s, are in the same equivalence class, i.e. t always has a response to s
        const auto universally_true = nfa.add_state();
        const auto universally_false = nfa.add_state();
        nfa.final.insert(universally_true);
        for (const auto& label : nfa.alphabet->get_alphabet_symbols()) {
            nfa.delta.add(universally_true, label, universally_true);
            nfa.delta.add(universally_false, label, universally_false);
        }
        for (int t = 0; t < lts.size(); ++t) {
            for (int s = 0; s < lts.size(); ++s) {
                if (rel.simulates(t, s)) {
                    state_pair_to_nfa_state[s][t] = universally_true;
                // } else if (rel.never_simulates(t, s)) {
                //     state_pair_to_nfa_state[s][t] = universally_false;
                } else {
                    state_pair_to_nfa_state[s][t] = nfa.add_state();
                    if (!lts.is_goal(s) || lts.is_goal(t)) {
                        nfa.final.insert(state_pair_to_nfa_state[s][t]);
                    }
                }
            }
        }

        for (int t = 0; t < lts.size(); ++t) {
            for (int s = 0; s < lts.size(); ++s) {
                if (state_pair_to_nfa_state[s][t] != universally_true) {
                    const auto& s_transitions = lts.get_transitions(s);
                    const auto& t_transitions = lts.get_transitions(t);
                    std::unordered_set<int> unused_labels(all_labels.begin(), all_labels.end());
                    for (const auto& s_tr : s_transitions) {
                        if (!lts.is_relevant_label_group(s_tr.label_group)) {
                            for (auto l : lts.get_labels(s_tr.label_group)) {
                                unused_labels.erase(l);
                                nfa.delta.add(state_pair_to_nfa_state[s][t], l, universally_true);
                            }
                            continue;
                        }

                        for (auto s_label : lts.get_labels(s_tr.label_group)) {
                            unused_labels.erase(s_label);
                            mata::nfa::StateSet t_targets;
                            for (const auto t_tr : t_transitions) {
                                for (auto t_label : lts.get_labels(t_tr.label_group)) {
                                    if (label_relation.label_dominates_label_in_all_other(factor, t_label, s_label)) {
                                        t_targets.insert(state_pair_to_nfa_state.at(s_tr.target).at(t_tr.target));
                                    }
                                }
                            }

                            if (label_relation.noop_simulates_label_in_all_other(factor, s_label)) {
                                t_targets.insert(state_pair_to_nfa_state.at(s_tr.target).at(t));
                            }

                            if (t_targets.empty()) {
                                // No t-responses, add a transition to universally false
                                nfa.delta.add(state_pair_to_nfa_state[s][t], s_label, universally_false);
                            } else if (t_targets.contains(universally_true)) {
                                // If there is a t-response to a state that simulates s', only add that transition as it is better than the others
                                nfa.delta.add(state_pair_to_nfa_state[s][t], s_label, universally_true);
                            } else {
                                if (under_approximate) {
                                    /* We can only pick one response of t, so we should try and pick the best one, i.e. the one that simulates most plans of s
                                     * Prefer final states
                                     * Prefer states where more transitions lead to universally true
                                     */
                                    mata::nfa::State best_target = *t_targets.begin();
                                    double best_target_score = 0.0;
                                    for (const auto& target : t_targets) {
                                        size_t total_transitions = 0;
                                        size_t transitions_to_true = 0;
                                        size_t transitions_to_false = 0;
                                        for (auto sp : nfa.delta.state_post(target)) {
                                            for (auto tt : sp) {
                                                if (tt == universally_true) {
                                                    ++transitions_to_true;
                                                } else if (tt == universally_false) {
                                                    ++transitions_to_false;
                                                }
                                            }
                                            ++total_transitions;
                                        }

                                        double target_score = ((double)(transitions_to_true - transitions_to_false) / total_transitions) + (nfa.final.contains(target)? 100.0: 0.0);
                                        if (target_score > best_target_score) {
                                            best_target = target;
                                            best_target_score = target_score;
                                        }
                                    }
                                    nfa.delta.add(state_pair_to_nfa_state[s][t], s_label, best_target);
                                } else {
                                    nfa.delta.add(state_pair_to_nfa_state[s][t], s_label, t_targets);
                                }
                            }
                        }
                    }
                    for (const auto& label : unused_labels) {
                        // If there is no s-transition for a label, t can trivially simulate it
                        nfa.delta.add(state_pair_to_nfa_state[s][t], label, universally_true);
                    }
                }
            }
        }
        return {nfa, state_pair_to_nfa_state};
    }

    void QualifiedDominanceConstraints::add_automaton_to_lp(const mata::nfa::Nfa& automaton, lp::LinearProgram& lp, mata::nfa::State state, int factor) {
        LPVariables& lp_variables = lp.get_variables();
        LPConstraints& lp_constraints = lp.get_constraints();

        // Add init variable
        lp_variables.emplace_back(lp::LPVariable(0., lp.get_infinity(), 0., false));
        auto i_var = lp_variables.size() - 1;
#ifndef NDEBUG
        lp_variables.set_name(i_var, std::format("I[q={},f={}]", state, factor));
#endif
        init_variables.at(factor).emplace_back(lp_variables.size() - 1);

        // Make sure to make transition variables vector, even if it is trivially empty
        transition_variables.at(factor).emplace_back();

        // If the language of automaton is empty, i.e. there is no final state, add unsolvable constraint when i_var >= 1
        if (automaton.final.empty()) {
            lp::LPConstraint unsolvable_constraint(0., lp.get_infinity());
            unsolvable_constraint.insert(i_var, -1.);
            lp_constraints.push_back(unsolvable_constraint);
#ifndef NDEBUG
            lp_constraints.set_name(lp_constraints.size() - 1, std::format("Unsolvable[q={},f={}]", state, factor));
#endif
            return;
        } else if (only_pruning) {
            // If we are only doing pruning, then add no constraints for non-dominated states
            return;
        }

        // Add transition variables and collect ingoing/outgoing transitions for each state
        std::vector<std::set<int>> state_ingoing(automaton.num_of_states(), std::set<int>());
        std::vector<std::set<int>> state_outgoing(automaton.num_of_states(), std::set<int>());
        std::vector<std::vector<int>> label_transitions(automaton.alphabet->get_alphabet_symbols().size(), std::vector<int>());

        auto transitions = automaton.delta.transitions();
        auto it = transitions.begin();
        for (int j = 0; it != transitions.end(); ++j, ++it) {
            const auto& [from, label, to] = *it;

            // Add transition variable
            lp_variables.emplace_back(lp::LPVariable(0., lp.get_infinity(), 0., false));
            auto transition_variable = lp_variables.size() - 1;
#ifndef NDEBUG
            lp_variables.set_name(transition_variable, std::format("T[q={},f={},{}-{}>{}]", state, factor, from, std::to_string(label), to));
#endif
            transition_variables.at(factor).at(state).emplace_back();

            label_transitions.at(label).push_back(transition_variable);

            // Add this transition to the list of ingoing/outgoing transitions for the target/source states
            state_ingoing[to].insert(transition_variable);
            state_outgoing[from].insert(transition_variable);
        }


        // Add the flow constraint for each state
        for (size_t j = 0; j < automaton.num_of_states(); ++j) {
            // 0 <= Sum of ingoing transitions - sum of outgoing transitions + initial state <= goal state? 1: 0
            lp::LPConstraint flow_constraint(0., automaton.final.contains(j)? 1.: 0.);
#ifndef NDEBUG
            std::string flow_constraint_string = std::format("{} <= ", 0.);
#endif
            for (const auto& ingoing : state_ingoing[j]) {
                if (!state_outgoing[j].contains(ingoing)) {
                    flow_constraint.insert(ingoing, 1.);
#ifndef NDEBUG
                    flow_constraint_string += std::format("+{} ", lp_variables.get_name(ingoing));
#endif
                }
            }
            for (const auto& outgoing : state_outgoing[j]) {
                if (!state_ingoing[j].contains(outgoing)) {
                    flow_constraint.insert(outgoing, -1.);
#ifndef NDEBUG
                    flow_constraint_string += std::format("-{} ", lp_variables.get_name(outgoing));
#endif
                }
            }
            if (automaton.initial.contains(j)) {
                flow_constraint.insert(init_variables[factor][state], 1.);
#ifndef NDEBUG
                flow_constraint_string += std::format("+{} ", lp_variables.get_name(init_variables[factor][state]));
#endif
            }
            lp_constraints.push_back(flow_constraint);
#ifndef NDEBUG
            flow_constraint_string += std::format("<= {}", automaton.final.contains(j)? 1.: 0.);
            lp_constraints.set_name(lp_constraints.size() - 1, std::format("Flow[q={},f={},q'={}] {}", state, factor, j, flow_constraint_string));
#endif
        }

        // Add operator count constraints for each label
        for (auto [l, transition_variables] : std::views::enumerate(label_transitions)) {
            lp::LPConstraint label_transition_constraint(0., 0);

            label_transition_constraint.insert(l, 1);

            for (auto t_var : transition_variables) {
                label_transition_constraint.insert(t_var, -1.);
            }
            lp_constraints.push_back(label_transition_constraint);
#ifndef NDEBUG
            lp_constraints.set_name(lp_constraints.size() - 1, std::format("Label {}", l));
#endif
        }
    }

    void draw_nfa(const std::string& file_name, const mata::nfa::Nfa& nfa, const fts::LabelledTransitionSystem& lts, const std::vector<std::vector<unsigned long>>& state_pair_to_nfa_state) {
        graphviz::Graph g(true);
        for (size_t i = 0; i < nfa.num_of_states(); ++i) {
            std::vector<std::string> state_pairs = {std::format("{}", i)};
            for (int s = 0; s < lts.size(); ++s) {
                for (int t = 0; t < lts.size(); ++t) {
                    if (state_pair_to_nfa_state[s][t] == i) {
                        state_pairs.emplace_back(std::format("{} <= {}", lts.state_name(s), lts.state_name(t)));
                    }
                }
            }
            g.add_node(state_pairs.empty()? std::format("{}", i): boost::algorithm::join(state_pairs, "\n"), nfa.final.contains(i)? "peripheries=2": "");
        }
        for (size_t s = 0; s < nfa.num_of_states(); ++s) {
            std::unordered_map<int, std::vector<int>> state_labels;
            for (const mata::nfa::SymbolPost& symbol_posts : nfa.delta.state_post(s)) {
                for (const auto& t : symbol_posts.targets) {
                    state_labels[t].push_back(symbol_posts.symbol);
                }
            }
            for (const auto& [t, labels] : state_labels) {
                g.add_edge(s, t, lts.fact_value_names.get_common_operators_name(labels));
            }
        }

        g.output_graph(file_name);
    }

    void QualifiedDominanceConstraints::initialize_constraints(const std::shared_ptr<AbstractTask>& task, lp::LinearProgram& lp) {
        fts::AtomicTaskFactory fts_factory;
        transformed_task = std::make_unique<TransformedFTSTask>(fts_factory.transform_to_fts(task));

#ifndef NDEBUG
        for (int l = 0; l < task->get_num_operators(); l++) {
            std::println("cost({}) = {}", task->get_operator_name(l, false), task->get_operator_cost(l, false));
        }
        fts::draw_fts("fts.dot", *transformed_task->fts_task);
#endif

        factored_qdomrel = qdominance::QualifiedLDSimulation(utils::Verbosity::DEBUG).compute_dominance_relation(*transformed_task->fts_task);

#ifndef NDEBUG
        factored_qdomrel->print_simulations();
        factored_qdomrel->print_label_dominance();
#endif

        std::println("Number of simulations: ", factored_qdomrel->num_simulations());
        std::println("Percentage simulations: ", factored_qdomrel->get_percentage_simulations(false));

        // Create all the LP variables
        for (size_t i = 0; i < factored_qdomrel->size(); ++i) {
            const auto& lts = transformed_task->fts_task->get_factor(i);
            auto [automaton, local_state_pair_to_nfa_state] = construct_transition_response_nfa(i, lts, (*factored_qdomrel)[i], factored_qdomrel->get_label_relation(), approximate_determinization);
#ifndef NDEBUG
            draw_nfa(std::format("nfa_premin_{}.dot", i), automaton, lts, local_state_pair_to_nfa_state);
#endif
            if (minimize_nfa) {
                std::println("Automaton size before minimization for factor {}: {}", i, automaton.num_of_states());
                auto [minimal_automaton, state_to_reduced_map] = qdominance::merge_non_differentiable_states(automaton, approximate_determinization);
                for (int s = 0; s < lts.size(); ++s) {
                    for (int t = 0; t < lts.size(); ++t) {
                        local_state_pair_to_nfa_state[s][t] = state_to_reduced_map[local_state_pair_to_nfa_state[s][t]];
                    }
                }
                minimal_automaton.swap_final_nonfinal(); // Swap final and non-final states; nfa should be deterministic and complete
                automaton = minimal_automaton;
            } else {
                automaton.make_complete();
                if (!approximate_determinization) {
                    automaton = qdominance::determinize(automaton);
                }
                automaton.swap_final_nonfinal(); // Swap final and non-final states; nfa should be deterministic and complete
            }
            std::println("Automaton size for factor {}: {}", i, automaton.num_of_states());
            state_pair_to_nfa_state.push_back(local_state_pair_to_nfa_state);

#ifndef NDEBUG

            std::println("Automaton final states: {}", automaton.final.size());
            draw_nfa(std::format("nfa_{}.dot", i), automaton, lts, local_state_pair_to_nfa_state);

            // std::println("Label groups for factor {}", i);
            // for (auto [j, lg] : std::views::enumerate(lts.get_label_groups())) {
            //     std::print("{}: ", j);
            //     for (auto label : lg) {
            //         std::print("{}, ", lts.label_name(label));
            //     }
            //     std::println(" ; {}", lts.fact_value_names.get_common_operators_name(lg));
            // }
#endif
            init_variables.emplace_back();
            transition_variables.emplace_back();
            for (size_t q = 0; q < automaton.num_of_states(); ++q) {
                automaton.initial = {static_cast<mata::nfa::State>(q)};
                mata::nfa::Nfa reduced_automaton;
                if (minimize_nfa) {
                    reduced_automaton = minimize(automaton);
                    reduced_automaton.alphabet = automaton.alphabet;
                    reduced_automaton.make_complete();
                } else {
                    reduced_automaton = automaton;
                }

                // draw_nfa(std::format("reduced_automaton_{}", q), reduced_automaton, lts, local_state_pair_to_nfa_state);
                add_automaton_to_lp(reduced_automaton, lp, q, i);
            }
        }

#ifndef NDEBUG
        // for (int i = 0; i < lp.get_constraints().size(); ++i) {
        //     std::cout << lp.get_constraints().get_name(i) << std::endl;
        // }
#endif
    }

    bool QualifiedDominanceConstraints::update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) {
        state.unpack();
        auto explicit_state = state.get_unpacked_values();

        LPConstraints lp_constraints;

#ifndef NDEBUG
        // Print state
        std::print("g-value: {}, ", g_value);
        for (size_t i = 0; i < explicit_state.size(); ++i) {
            std::cout << transformed_task->fts_task->get_factor(i).state_name(explicit_state.at(i)) << ", ";
        }
        std::cout << std::endl;
#endif

        if (!std::ranges::all_of(std::views::enumerate(explicit_state), [&](std::pair<int, int> fs) { return transformed_task->fts_task->get_factor(fs.first).is_goal(fs.second); })) {
            // If not a goal state, add a constraint that some operator must be applied
            lp::LPConstraint any_label_constraint(1., lp_solver.get_infinity());
            for (int l = 0; l < transformed_task->fts_task->get_num_labels(); ++l) {
                any_label_constraint.insert(l, 1.);
            }
            lp_constraints.push_back(any_label_constraint);
#ifndef NDEBUG
            lp_constraints.set_name(lp_constraints.size() - 1, "AnyLabel");
#endif
        }

        lp_solver.write_lp("lp.lp");

        // The constraints for each previous state, the set of initial states for each factor such that one of them must
        // reach a goal state to satisfy the constraint.
        std::vector<std::vector<mata::nfa::State>> init_state_constraints;

        for (const auto& previous_state : previous_states) {
            if (previous_state.g_value <= g_value) {
                std::vector<mata::nfa::State> initial_states;
                bool same_state = previous_state.g_value == g_value;
                size_t num_dominations = 0;
                for (size_t i = 0; i < previous_state.state.size(); ++i) {
                    // auto fvn = (*factored_qdomrel)[i].fact_value_names;
                    if (factored_qdomrel->get_local_relations()[i]->simulates(previous_state.state[i], explicit_state[i]))
                        num_dominations += 1;
#ifndef NDEBUG
                    std::cout << "    Adding " << state_pair_to_nfa_state.at(i).at(explicit_state[i]).at(previous_state.state[i]) << ": " << transformed_task->fts_task->get_factor(i).state_name(explicit_state[i]) << " <= " << transformed_task->fts_task->get_factor(i).state_name(previous_state.state[i]) << std::endl;
#endif
                    initial_states.push_back(state_pair_to_nfa_state.at(i).at(explicit_state[i]).at(previous_state.state[i]));
                    same_state &= explicit_state[i] == previous_state.state[i];
                }
                if (same_state) {
                    std::println("Break");
                    // If the state is the same as the previous state, stop here, as we cannot allow comparing against
                    // nodes that are later or equal in the evaluation order.
                    break;
                }
                if (num_dominations == factored_qdomrel->size()) {
                    // If all factors dominate the previous state, then we can prune this state
                    return true;
                } if (num_dominations == factored_qdomrel->size() - 1) {
                    init_state_constraints.emplace_back(initial_states);
                }
            }
        }

        if (!init_state_constraints.empty()) {
            // For each set of constraints
            for (size_t i = 0; i < init_state_constraints.size(); ++i) {
                // Add initial state constraints
                lp::LPConstraint constraint(1., lp_solver.get_infinity());
                for (size_t j = 0; j < init_state_constraints.at(i).size(); ++j) {
                    // Each initial state should appear with coefficient |constraints|
                    constraint.insert(init_variables.at(j).at(init_state_constraints.at(i).at(j)), 1.);
                }
                lp_constraints.push_back(constraint);
#ifndef NDEBUG
                lp_constraints.set_name(lp_constraints.size() - 1, std::format("InitStateConstraint[{}]", i));
#endif
            }
        }

        lp_solver.add_temporary_constraints(lp_constraints);

        previous_states.emplace_back(explicit_state, g_value);
        return false;
    }


    class QualifiedDominanceConstraintsFeature
    : public plugins::TypedFeature<ConstraintGenerator, QualifiedDominanceConstraints> {
    public:
        QualifiedDominanceConstraintsFeature() : TypedFeature("qualified_dominance_constraints") {
            document_title("Qualified Dominance constraints");
            document_synopsis("");
            add_option<bool>("only_pruning", "Only use dominance to assign h=∞", "false");
            add_option<bool>("minimize_nfa", "Minimize the NFA before adding it to the LP", "true");
            add_option<bool>("approx_det", "Under-approximate the determinization of the transition response NA", "false");
        }

        [[nodiscard]] std::shared_ptr<QualifiedDominanceConstraints> create_component(const plugins::Options &opts, const utils::Context &) const override {
            return std::make_shared<QualifiedDominanceConstraints>(opts.get<bool>("only_pruning"), opts.get<bool>("minimize_nfa"), opts.get<bool>("approx_det"));
        }
    };

    static plugins::FeaturePlugin<QualifiedDominanceConstraintsFeature> _plugin;
}
