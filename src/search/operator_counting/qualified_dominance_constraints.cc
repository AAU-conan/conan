#include "qualified_dominance_constraints.h"

#include "delete_relaxation_if_constraints.h"
#include "../qualified_dominance/qualified_dominance_pruning_local.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../factored_transition_system/draw_fts.h"
#include "../plugins/plugin.h"

#include <print>

namespace operator_counting {

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


    void QualifiedDominanceConstraints::initialize_constraints(const std::shared_ptr<AbstractTask>& task, lp::LinearProgram& lp) {
        fts::AtomicTaskFactory fts_factory;
        const auto transformed_task = fts_factory.transform_to_fts(task);

#ifndef NDEBUG
        fts::draw_fts("fts.dot", *transformed_task.fts_task);
#endif

        factored_qdomrel = qdominance::QualifiedLDSimulation(utils::Verbosity::DEBUG).compute_dominance_relation(*transformed_task.fts_task);

        for (int i = 0; i < factored_qdomrel->size(); ++i) {
            assert((*factored_qdomrel)[i].get_nfa().final.size() == (*factored_qdomrel)[i].get_nfa().num_of_states() - 1);
        }

        LPVariables& lp_variables = lp.get_variables();
        LPConstraints& lp_constraints = lp.get_constraints();

        auto safe_push_constraint = [&lp, &lp_variables, &lp_constraints](const lp::LPConstraint& c) {
            for (auto v : c.get_variables()) {
                assert(v < lp.get_variables().size());
            }
            assert(std::ranges::to<std::set<int>>(c.get_variables()).size() == c.get_variables().size());
            lp_constraints.push_back(c);
        };

        // Create all the LP variables
        for (int i = 0; i < factored_qdomrel->size(); ++i) {
            auto [automaton, label_groups] = label_group_nfa((*factored_qdomrel)[i].get_nfa());
            automaton.swap_final_nonfinal(); // Swap final and non-final states; automaton should already be deterministic and completes

            auto fvn = (*factored_qdomrel)[i].fact_value_names;

            // for (auto [i, lg] : std::views::enumerate(label_groups)) {
            //     std::cout << "Label group " << i << ": ";
            //     for (auto label : lg) {
            //         std::cout << fvn.get_operator_name(label, false) << " ";
            //     }
            //     std::cout << std::endl;
            // }

            init_variables.emplace_back();
            transition_variables.emplace_back();
            for (int q = 0; q < automaton.num_of_states(); ++q) {
                automaton.initial = {static_cast<mata::nfa::State>(q)};
                auto reduced_automaton = minimize(automaton);
                reduced_automaton.alphabet = automaton.alphabet;

                // Add init variable
                lp_variables.emplace_back(lp::LPVariable(0., lp.get_infinity(), 0., false));
                auto i_var = lp_variables.size() - 1;
#ifndef NDEBUG
                lp_variables.set_name(i_var, std::format("I^{}_{}", q, i));
#endif
                init_variables[i].emplace_back(lp_variables.size() - 1);

                // Make sure to make transition variables vector, even if it is trivially empty
                transition_variables.at(i).emplace_back();

                // If the language of reduced_automaton is empty, i.e. there is no final state, add unsolvable constraint when i_var >= 1
                if (reduced_automaton.final.empty()) {
                    lp::LPConstraint unsolvable_constraint(0., lp.get_infinity());
                    unsolvable_constraint.insert(i_var, -1.);
                    safe_push_constraint(unsolvable_constraint);
                }

                // Add transition variables and collect ingoing/outgoing transitions for each state
                std::vector<std::vector<int>> state_ingoing(reduced_automaton.num_of_states(), std::vector<int>());
                std::vector<std::vector<int>> state_outgoing(reduced_automaton.num_of_states(), std::vector<int>());
                std::vector<std::vector<int>> label_group_transitions(reduced_automaton.alphabet->get_alphabet_symbols().size(), std::vector<int>());

                auto transitions = reduced_automaton.delta.transitions();
                auto it = transitions.begin();
                for (int j = 0; it != transitions.end(); ++j, ++it) {
                    const auto& [from, label, to] = *it;

                    if (from == to)
                        continue; // Skip self-loops, they mess with flow constraints because the coefficients will be set twice, both for ingoing and outgoing transitions

                    // Add transition variable
                    lp_variables.emplace_back(lp::LPVariable(0., lp.get_infinity(), 0., false));
                    auto transition_variable = lp_variables.size() - 1;
#ifndef NDEBUG
                    // lp_variables.set_name(transition_variable, std::format("T^{}_{}_{}-{}>{}", q, i, from, fvn.get_operator_name(label, false), to));
#endif
                    transition_variables.at(i).at(q).emplace_back();

                    // Add this transition to the list of transitions for the label
                    label_group_transitions[label].push_back(transition_variable);

                    // Add this transition to the list of ingoing/outgoing transitions for the target/source states
                    state_ingoing[to].push_back(transition_variable);
                    state_outgoing[from].push_back(transition_variable);
                }

                // Add the flow constraint for each state
                for (int j = 0; j < reduced_automaton.num_of_states(); ++j) {
                    // Sum of ingoing transitions - sum of outgoing transitions + initial state - goal state >= 0
                    lp::LPConstraint flow_constraint(0., lp.get_infinity());
                    for (const auto& ingoing : state_ingoing[j]) {
                        flow_constraint.insert(ingoing, 1.);
                    }
                    for (const auto& outgoing : state_outgoing[j]) {
                        flow_constraint.insert(outgoing, -1.);
                    }
                    double init_var_coef = 0.0;
                    if (reduced_automaton.final.contains(j)) {
                        // If this is a final state, subtract flow to state
                        init_var_coef -= 1.0;
                    }
                    if (reduced_automaton.initial.contains(j)) {
                        // Add initial state variable flow
                        init_var_coef += 1.0;
                    }
                    if (init_var_coef != 0.0) {
                        flow_constraint.insert(init_variables[i][q], init_var_coef);
                    }
                    safe_push_constraint(flow_constraint);
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
                    safe_push_constraint(label_constraint);
                }
            }
        }
    }

    bool QualifiedDominanceConstraints::update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) {
        state.unpack();
        auto explicit_state = state.get_unpacked_values();

        // The constraints for each previous state, the set of initial states for each factor such that one of them must
        // reach a goal state to satisfy the constraint.
        std::vector<std::vector<mata::nfa::State>> init_state_constraints;

        for (const auto& previous_state : previous_states) {
            if (previous_state.g_value <= g_value) {
                std::vector<mata::nfa::State> initial_states;
                for (int i = 0; i < previous_state.state.size(); ++i) {
                    auto fvn = (*factored_qdomrel)[i].fact_value_names;
                    // std::cout << "Adding " << fvn.get_fact_value_name(previous_state.state[i]) << " " << fvn.get_fact_value_name(explicit_state[i]) << std::endl;
                    initial_states.push_back((*factored_qdomrel)[i].nfa_simulates(previous_state.state[i], explicit_state[i]));
                }
                init_state_constraints.emplace_back(initial_states);
            }
        }

        if (!init_state_constraints.empty()) {
            LPConstraints lp_constraints;
            // For each set of constraints
            for (int i = 0; i < init_state_constraints.size(); ++i) {
                // Add initial state constraints
                lp::LPConstraint constraint(1., lp_solver.get_infinity());
                for (int j = 0; j < init_state_constraints.at(i).size(); ++j) {
                    // Each initial state should appear with coefficient |constraints|
                    constraint.insert(init_variables.at(j).at(init_state_constraints.at(i).at(j)), 1.);
                }
                lp_constraints.push_back(constraint);
            }
            lp_solver.add_temporary_constraints(lp_constraints);
        }

        previous_states.emplace_back(explicit_state, g_value);
        return false;
    }


    class QualifiedDominanceConstraintsFeature
    : public plugins::TypedFeature<ConstraintGenerator, QualifiedDominanceConstraints> {
    public:
        QualifiedDominanceConstraintsFeature() : TypedFeature("qualified_dominance_constraints") {
            document_title("Qualified Dominance constraints");
            document_synopsis("");
        }

        [[nodiscard]] std::shared_ptr<QualifiedDominanceConstraints> create_component(const plugins::Options &, const utils::Context &) const override {
            return std::make_shared<QualifiedDominanceConstraints>();
        }
    };

    static plugins::FeaturePlugin<QualifiedDominanceConstraintsFeature> _plugin;
}
