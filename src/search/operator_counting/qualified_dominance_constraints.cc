#include "qualified_dominance_constraints.h"

#include "../qualified_dominance/qualified_dominance_pruning_local.h"
#include "../qualified_dominance/nfa_merge_non_differentiable.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../factored_transition_system/draw_fts.h"
#include "../plugins/plugin.h"

#include <print>

#include "delete_relaxation_if_constraints.h"
#include "../utils/graphviz.h"

namespace operator_counting {

    [[nodiscard]] std::pair<mata::nfa::Nfa,std::vector<std::vector<mata::nfa::State>>> construct_transition_response_nfa(const int factor, const LabelledTransitionSystem& lts, const qdominance::QualifiedLocalStateRelation2& rel, const qdominance::QualifiedLabelRelation& label_relation) {
        mata::nfa::Nfa nfa;
        auto all_label_groups = std::views::iota(0, lts.get_num_label_groups()) | std::ranges::to<std::vector>();
        nfa.alphabet = new mata::EnumAlphabet(all_label_groups.begin(), all_label_groups.end());
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
                    std::unordered_set<int> unused_labels(all_label_groups.begin(), all_label_groups.end());
                    for (const auto& s_tr : s_transitions) {
                        unused_labels.erase(s_tr.label_group.group);
                        if (!lts.is_relevant_label_group(s_tr.label_group)) {
                            // If the label group is not relevant, t can trivially simulate it
                            continue;
                        }

                        mata::nfa::StateSet t_targets;
                        for (const auto t_tr : t_transitions) {
                            if (label_relation.simulates_in_all_other(factor, t_tr.label_group, s_tr.label_group)) {
                                t_targets.insert(state_pair_to_nfa_state.at(s_tr.target).at(t_tr.target));
                            }
                        }

                        if (label_relation.noop_simulates_in_all_other(factor, s_tr.label_group)) {
                            t_targets.insert(state_pair_to_nfa_state.at(s_tr.target).at(t));
                        }

                        if (t_targets.empty()) {
                            // No t-responses, add a transition to universally false
                            nfa.delta.add(state_pair_to_nfa_state[s][t], s_tr.label_group.group, universally_false);
                        } else if (t_targets.contains(universally_true)) {
                            // If there is a t-response to a state that simulates s', only add that transition as it is better than the others
                            nfa.delta.add(state_pair_to_nfa_state[s][t], s_tr.label_group.group, universally_true);
                        } else {
                            nfa.delta.add(state_pair_to_nfa_state[s][t], s_tr.label_group.group, t_targets);
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

    [[nodiscard]] std::pair<mata::nfa::Nfa, std::vector<std::vector<int>>> label_group_nfa(const mata::nfa::Nfa& nfa) {
        // Start with all labels in the same group
        std::vector<int> label_to_group(nfa.alphabet->get_alphabet_symbols().size(), 0);
        int next_label_group = 1;

        // Iterate through all transitions and split the group into two group where one has the transition and another doesn't
        for (const auto& [from, label, to] : nfa.delta.transitions()) {
            int lg = label_to_group[label];
            for (const auto& symbol_post : nfa.delta.state_post(from)) {
                if (label_to_group.at(symbol_post.symbol) != lg)
                    continue;

                for (const auto& tgt : symbol_post.targets) {
                    if (tgt == to) {
                        // Put all labels in this label group that have this transition in the next group
                        label_to_group.at(symbol_post.symbol) = next_label_group;
                    }
                }
            }
            ++next_label_group;
        }

        // The label groups may be sparse now, so compact them
        auto used_groups = label_to_group | std::ranges::to<std::set<int>>();
        std::vector<std::vector<int>> label_groups(used_groups.size());

        for (auto [ng, og] : std::views::enumerate(used_groups)) {
            for (const auto& [label, lg] : std::views::enumerate(label_to_group)) {
                if (lg == og) {
                    label_groups[ng].push_back(label);
                    label_to_group[label] = ng;
                }
            }
        }

        mata::nfa::Nfa new_nfa(nfa.num_of_states(), mata::nfa::StateSet(nfa.initial), mata::nfa::StateSet(nfa.final), nullptr);
        auto all_labels = std::views::iota(0ul, used_groups.size());
        new_nfa.alphabet = new mata::EnumAlphabet(all_labels.begin(), all_labels.end());
        for (const auto& [from, label, to] : nfa.delta.transitions()) {
            new_nfa.delta.add(from, label_to_group[label], to);
        }

        return std::make_pair(new_nfa, label_groups);
    }

    [[nodiscard]] mata::nfa::Nfa lts_to_nfa(const fts::LabelledTransitionSystem& lts) {
        mata::nfa::Nfa nfa;

        for (int s = 0; s < lts.size(); ++s) {
            nfa.add_state(s);
            if (lts.is_goal(s)) {
                nfa.final.insert(static_cast<mata::nfa::State>(s));
            }
        }

        for (const auto& t : lts.get_transitions()) {
            for (const auto label : lts.get_labels(t.label_group)) {
                nfa.delta.add(t.src, label, t.target);
            }
        }

        return nfa;
    }

    void QualifiedDominanceConstraints::add_automaton_to_lp(const mata::nfa::Nfa& automaton, lp::LinearProgram& lp, mata::nfa::State state, int factor, const std::vector<std::vector<int>>& label_groups) {
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
        std::vector<std::vector<int>> label_group_transitions(automaton.alphabet->get_alphabet_symbols().size(), std::vector<int>());

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

            // Add this transition to the list of transitions for the label
            label_group_transitions[label].push_back(transition_variable);

            // Add this transition to the list of ingoing/outgoing transitions for the target/source states
            state_ingoing[to].insert(transition_variable);
            state_outgoing[from].insert(transition_variable);
        }

        // Add the flow constraint for each state
        for (int j = 0; j < automaton.num_of_states(); ++j) {
            // Sum of ingoing transitions - sum of outgoing transitions + initial state - goal state >= 0
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

        // Add operator count constraints for each label group
        for (auto [lg_i, lg] : std::views::enumerate(label_groups)) {
            lp::LPConstraint label_constraint(0., lp.get_infinity());
            for (auto label : lg) {
                label_constraint.insert(label, 1.);
            }
            for (const auto& transition : label_group_transitions.at(lg_i)) {
                label_constraint.insert(transition, -1.);
            }
            lp_constraints.push_back(label_constraint);
#ifndef NDEBUG
            lp_constraints.set_name(lp_constraints.size() - 1, std::format("LabelGroup[q={},f={},lg={}]", state, factor, lg_i));
#endif
        }
    }

    void draw_nfa(const std::string& file_name, const mata::nfa::Nfa& nfa, const fts::LabelledTransitionSystem& lts, const std::vector<std::vector<unsigned long>>& state_pair_to_nfa_state) {
        graphviz::Graph g(true);
        for (size_t i = 0; i < nfa.num_of_states(); ++i) {
            std::vector<std::string> state_pairs;
            for (size_t s = 0; s < lts.size(); ++s) {
                for (size_t t = 0; t < lts.size(); ++t) {
                    if (state_pair_to_nfa_state[s][t] == i) {
                        state_pairs.emplace_back(std::format("{} <= {}", lts.state_name(s), lts.state_name(t)));
                    }
                }
            }
            g.add_node(state_pairs.empty()? std::format("{}", i): boost::algorithm::join(state_pairs, "\n"), nfa.final.contains(i)? "peripheries=2": "");
        }
        for (auto const& [from, label, to] : nfa.delta.transitions()) {
            g.add_edge(from, to, lts.label_group_name(LabelGroup(label)));
        }
        g.output_graph(file_name);
    }

    void QualifiedDominanceConstraints::initialize_constraints(const std::shared_ptr<AbstractTask>& task, lp::LinearProgram& lp) {
        fts::AtomicTaskFactory fts_factory;
        transformed_task = std::make_unique<TransformedFTSTask>(fts_factory.transform_to_fts(task));

#ifndef NDEBUG
        fts::draw_fts("fts.dot", *transformed_task->fts_task);
#endif

        factored_qdomrel = qdominance::QualifiedLDSimulation(utils::Verbosity::DEBUG).compute_dominance_relation(*transformed_task->fts_task);

#ifndef NDEBUG
        // for (int i = 0; i < factored_qdomrel->size(); ++i) {
        //     std::println("Factor {}", i);
        //     graphviz::Graph graph;
        //     const auto& rel = *factored_qdomrel->get_local_relations().at(i);
        //     const auto& lts = transformed_task.fts_task->get_factor(i);
        //
        //     auto no_response_node = graph.add_node("no response", "shape=doublecircle");
        //
        //     std::map<std::pair<int,int>,size_t> state_pair_to_node;
        //     for (int s = 0; s < lts.size(); ++s) {
        //         for (int t = 0; t < lts.size(); ++t) {
        //             state_pair_to_node[{s,t}] = graph.add_node(std::format("{} <= {}:", lts.state_name(s), lts.state_name(t)));
        //         }
        //     }
        //     for (int s = 0; s < lts.size(); ++s) {
        //         for (int t = 0; t < lts.size(); ++t) {
        //             std::println("{} <= {}:", lts.state_name(s), lts.state_name(t));
        //             for (const auto& [s_tr_i, t_tr_is] : std::views::enumerate(rel.transition_responses[s][t])) {
        //                 auto s_tr = lts.get_transitions(s).at(s_tr_i);
        //                 std::println("    {} --{}-> {}", lts.state_name(s_tr.src), lts.label_group_name(s_tr.label_group), lts.state_name(s_tr.target));
        //                 if (t_tr_is.empty()) {
        //                     graph.add_edge(state_pair_to_node[{s,t}], no_response_node, std::format("{}<>none", lts.label_group_name(s_tr.label_group)));
        //                 }
        //                 for (auto t_tr_i : t_tr_is) {
        //                     if (t_tr_i == qdominance::NOOP_TRANSITION) {
        //                         std::println("        noop");
        //                         graph.add_edge(state_pair_to_node[{s,t}], state_pair_to_node[{s_tr.target,t}], std::format("{}<>{}", lts.label_group_name(s_tr.label_group), "noop"));
        //                     } else {
        //                         auto t_tr = lts.get_transitions(t).at(t_tr_i);
        //                         std::println("        {} --{}-> {}", lts.state_name(t_tr.src), lts.label_group_name(t_tr.label_group), lts.state_name(t_tr.target));
        //                         graph.add_edge(state_pair_to_node[{s,t}], state_pair_to_node[{s_tr.target,t_tr.target}], std::format("{}<>{}", lts.label_group_name(s_tr.label_group), lts.label_group_name(t_tr.label_group)));
        //                     }
        //                 }
        //             }
        //         }
        //     }
        //     graph.output_graph(std::format("response_graph_{}.dot", i));
        // }
#endif

        // Create all the LP variables
        for (int i = 0; i < factored_qdomrel->size(); ++i) {
            const auto& lts = transformed_task->fts_task->get_factor(i);
            auto [automaton, local_state_pair_to_nfa_state] = construct_transition_response_nfa(i, lts, (*factored_qdomrel)[i], factored_qdomrel->get_label_relation());
            {
                auto [minimal_automaton, state_to_reduced_map] = qdominance::merge_non_differentiable_states(automaton);
                for (int s = 0; s < lts.size(); ++s) {
                    for (int t = 0; t < lts.size(); ++t) {
                        local_state_pair_to_nfa_state[s][t] = state_to_reduced_map[local_state_pair_to_nfa_state[s][t]];
                    }
                }
                minimal_automaton.swap_final_nonfinal(); // Swap final and non-final states; nfa should be deterministic and complete
                automaton = minimal_automaton;
            }
            state_pair_to_nfa_state.push_back(local_state_pair_to_nfa_state);

#ifndef NDEBUG
            draw_nfa(std::format("nfa_{}.dot", i), automaton, lts, local_state_pair_to_nfa_state);

            std::println("Label groups for factor {}", i);
            for (auto [j, lg] : std::views::enumerate(lts.get_label_groups())) {
                std::print("{}: ", j);
                for (auto label : lg) {
                    std::print("{}, ", lts.label_name(label));
                }
                std::println();
            }
#endif
            auto lts_nfa = lts_to_nfa(transformed_task->fts_task->get_factor(i));

            init_variables.emplace_back();
            transition_variables.emplace_back();
            for (int q = 0; q < automaton.num_of_states(); ++q) {
                automaton.initial = {static_cast<mata::nfa::State>(q)};
                auto reduced_automaton = minimize(automaton);
                reduced_automaton.alphabet = automaton.alphabet;
                reduced_automaton.make_complete();

                add_automaton_to_lp(reduced_automaton, lp, q, i, lts.get_label_groups());
            }
        }

#ifndef NDEBUG
        for (int i = 0; i < lp.get_constraints().size(); ++i) {
            std::cout << lp.get_constraints().get_name(i) << std::endl;
        }
#endif
    }

    bool QualifiedDominanceConstraints::update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) {
        state.unpack();
        auto explicit_state = state.get_unpacked_values();

        LPConstraints lp_constraints;

#ifndef NDEBUG
        // Print state
        for (int i = 0; i < explicit_state.size(); ++i) {
            std::cout << transformed_task->fts_task->get_factor(i).state_name(explicit_state[i]) << ", ";
        }
        std::cout << std::endl;
#endif

        // The constraints for each previous state, the set of initial states for each factor such that one of them must
        // reach a goal state to satisfy the constraint.
        std::vector<std::vector<mata::nfa::State>> init_state_constraints;

        for (const auto& previous_state : previous_states) {
            if (previous_state.g_value <= g_value) {
                std::vector<mata::nfa::State> initial_states;
                for (int i = 0; i < previous_state.state.size(); ++i) {
                    // auto fvn = (*factored_qdomrel)[i].fact_value_names;
#ifndef NDEBUG
                    std::cout << "Adding " << state_pair_to_nfa_state.at(i).at(explicit_state[i]).at(previous_state.state[i]) << ": " << transformed_task->fts_task->get_factor(i).state_name(explicit_state[i]) << " <= " << transformed_task->fts_task->get_factor(i).state_name(previous_state.state[i]) << std::endl;
#endif
                    initial_states.push_back(state_pair_to_nfa_state.at(i).at(explicit_state[i]).at(previous_state.state[i]));
                }
                init_state_constraints.emplace_back(initial_states);
            }
        }

        if (!init_state_constraints.empty()) {
            // For each set of constraints
            for (int i = 0; i < init_state_constraints.size(); ++i) {
                // Add initial state constraints
                lp::LPConstraint constraint(1., lp_solver.get_infinity());
                for (int j = 0; j < init_state_constraints.at(i).size(); ++j) {
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
        }

        [[nodiscard]] std::shared_ptr<QualifiedDominanceConstraints> create_component(const plugins::Options &opts, const utils::Context &) const override {
            return std::make_shared<QualifiedDominanceConstraints>(opts.get<bool>("only_pruning"));
        }
    };

    static plugins::FeaturePlugin<QualifiedDominanceConstraintsFeature> _plugin;
}
