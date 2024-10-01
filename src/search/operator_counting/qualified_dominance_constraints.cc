#include "qualified_dominance_constraints.h"

#include "delete_relaxation_if_constraints.h"
#include "../qualified_dominance/qualified_dominance_pruning_local.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../factored_transition_system/draw_fts.h"
#include "../plugins/plugin.h"

#include <print>

namespace operator_counting {
    void QualifiedDominanceConstraints::initialize_constraints(const std::shared_ptr<AbstractTask>& task, lp::LinearProgram& lp) {
        fts::AtomicTaskFactory fts_factory;
        const auto transformed_task = fts_factory.transform_to_fts(task);

        fts::draw_fts("fts.dot", *transformed_task.fts_task);

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
            const auto& automaton = (*factored_qdomrel)[i].get_nfa();
            auto fvn = (*factored_qdomrel)[i].fact_value_names;

            auto non_final_states = mata::utils::SparseSet(automaton.final);
            non_final_states.complement(automaton.num_of_states());
            assert(non_final_states.size() == 1);
            mata::nfa::State goal_state = *non_final_states.begin();

            goal_variables.emplace_back();
            init_variables.emplace_back();
            transition_variables.emplace_back();
            for (int q = 0; q < automaton.num_of_states(); ++q) {
                // Add goal variable
                lp_variables.emplace_back(lp::LPVariable(0., 1., 0., false));
                auto goal_var = lp_variables.size() - 1;
                lp_variables.set_name(goal_var, std::format("G^{}_{}", q, i));
                goal_variables[i].emplace_back(goal_var);

                // Add init variables for each state
                lp_variables.emplace_back(lp::LPVariable(0., lp.get_infinity(), 0., false));
                auto i_var = lp_variables.size() - 1;
                lp_variables.set_name(i_var, std::format("I^{}_{}", q, i));
                init_variables[i].emplace_back(lp_variables.size() - 1);

                // Add transition variables and collect ingoing/outgoing transitions for each state
                transition_variables.at(i).emplace_back();
                std::vector<std::vector<int>> state_ingoing(automaton.num_of_states(), std::vector<int>());
                std::vector<std::vector<int>> state_outgoing(automaton.num_of_states(), std::vector<int>());
                std::vector<std::vector<int>> label_transitions(automaton.alphabet->get_alphabet_symbols().size(), std::vector<int>());

                auto transitions = automaton.delta.transitions();
                auto it = transitions.begin();
                for (int j = 0; it != transitions.end(); ++j, ++it) {
                    const auto& [from, label, to] = *it;

                    if (from == to)
                        continue; // Skip self-loops, they mess with flow constraints because the coefficients will be set twice, both for ingoing and outgoing transitions

                    // Add transition variable
                    lp_variables.emplace_back(lp::LPVariable(0., lp.get_infinity(), 0., false));
                    auto transition_variable = lp_variables.size() - 1;
                    lp_variables.set_name(transition_variable, std::format("T^{}_{}_{}-{}>{}", q, i, from, fvn.get_operator_name(label, false), to));
                    transition_variables.at(i).at(q).emplace_back();

                    // Add this transition to the list of transitions for the label
                    label_transitions[label].push_back(transition_variable);

                    // Add this transition to the list of ingoing/outgoing transitions for the target/source states
                    state_ingoing[to].push_back(transition_variable);
                    state_outgoing[from].push_back(transition_variable);
                }

                // Add the flow constraint for each state
                for (int j = 0; j < automaton.num_of_states(); ++j) {
                    // Sum of ingoing transitions - sum of outgoing transitions + initial state - goal state >= 0
                    lp::LPConstraint flow_constraint(0., lp.get_infinity());
                    for (const auto& ingoing : state_ingoing[j]) {
                        flow_constraint.insert(ingoing, 1.);
                    }
                    for (const auto& outgoing : state_outgoing[j]) {
                        flow_constraint.insert(outgoing, -1.);
                    }
                    double init_var_coef = 0.0;
                    if (j == goal_state) {
                        // If this is a final state, subtract flow to state
                        init_var_coef -= 1.0;
                    }
                    if (j == q) {
                        // Add initial state variable flow
                        init_var_coef += 1.0;
                    }
                    if (init_var_coef != 0.0) {
                        flow_constraint.insert(init_variables[i][q], init_var_coef);
                    }
                    safe_push_constraint(flow_constraint);
                }

                // Add operator count constraints for each label
                for (int j = 0; j < automaton.alphabet->get_alphabet_symbols().size(); ++j) {
                    lp::LPConstraint label_constraint(0., lp.get_infinity());
                    label_constraint.insert(j, 1.);
                    for (const auto& transition : label_transitions.at(j)) {
                        label_constraint.insert(transition, -1.);
                    }
                    safe_push_constraint(label_constraint);
                }
            }
        }

        std::cout << "Number of lp variables: " << lp.get_variables().size() << std::endl;
    }

    bool QualifiedDominanceConstraints::update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) {
        state.unpack();
        auto explicit_state = state.get_unpacked_values();

        // The constraints for each previous state, the set of initial states for each factor such that one of them must
        // reach a goal state to satisfy the constraint.
        std::vector<std::vector<mata::nfa::State>> init_state_constraints;

        for (const auto& previous_state : std::views::reverse(previous_states)) {
            if (previous_state.g_value < g_value) {
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
