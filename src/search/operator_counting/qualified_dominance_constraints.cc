#include "qualified_dominance_constraints.h"

#include "../qualified_dominance/qualified_dominance_pruning_local.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../factored_transition_system/draw_fts.h"
#include "../plugins/plugin.h"

#include <print>

#include "delete_relaxation_if_constraints.h"

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
            lp::LPConstraint flow_constraint(0., lp.get_infinity());
            for (const auto& ingoing : state_ingoing[j]) {
                if (!state_outgoing[j].contains(ingoing))
                    flow_constraint.insert(ingoing, 1.);
            }
            for (const auto& outgoing : state_outgoing[j]) {
                if (!state_ingoing[j].contains(outgoing))
                    flow_constraint.insert(outgoing, -1.);
            }
            double init_var_coef = 0.0;
            if (automaton.final.contains(j)) {
                // If this is a final state, subtract flow to state
                init_var_coef -= 1.0;
            }
            if (automaton.initial.contains(j)) {
                // Add initial state variable flow
                init_var_coef += 1.0;
            }
            if (init_var_coef != 0.0) {
                flow_constraint.insert(init_variables[factor][state], init_var_coef);
            }
            lp_constraints.push_back(flow_constraint);
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
        }
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

        // Create all the LP variables
        for (int i = 0; i < factored_qdomrel->size(); ++i) {
            auto [automaton, label_groups] = label_group_nfa((*factored_qdomrel)[i].get_nfa());
            automaton.swap_final_nonfinal(); // Swap final and non-final states; nfa should be deterministic and complete

            auto fvn = (*factored_qdomrel)[i].fact_value_names;
#ifndef NDEBUG
            std::println("Label groups for factor {}", i);
            for (auto [j, lg] : std::views::enumerate(label_groups)) {
                std::print("{}: ", j);
                for (auto label : lg) {
                    std::print("{}, ", fvn.get_operator_name(label, false));
                }
                std::println();
            }
#endif
            auto lts_nfa = lts_to_nfa(transformed_task.fts_task->get_factor(i));

            init_variables.emplace_back();
            transition_variables.emplace_back();
            for (int q = 0; q < automaton.num_of_states(); ++q) {
                automaton.initial = {static_cast<mata::nfa::State>(q)};
                auto reduced_automaton = minimize(automaton);
                reduced_automaton.alphabet = automaton.alphabet;

                add_automaton_to_lp(reduced_automaton, lp, q, i, label_groups);
            }
        }
    }

    bool QualifiedDominanceConstraints::update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) {
        state.unpack();
        auto explicit_state = state.get_unpacked_values();

        LPConstraints lp_constraints;

#ifndef NDEBUG
        // Print state
        for (int i = 0; i < explicit_state.size(); ++i) {
            std::cout << (*factored_qdomrel)[i].fact_value_names.get_fact_value_name(explicit_state[i]) << ", ";
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
                    auto fvn = (*factored_qdomrel)[i].fact_value_names;
                    // std::cout << "Adding " << fvn.get_fact_value_name(previous_state.state[i]) << " " << fvn.get_fact_value_name(explicit_state[i]) << std::endl;
                    initial_states.push_back((*factored_qdomrel)[i].nfa_simulates(previous_state.state[i], explicit_state[i]));
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
