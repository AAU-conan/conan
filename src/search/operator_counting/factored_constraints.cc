#include "factored_constraints.h"
#include "../plugins/plugin.h"
#include "delete_relaxation_if_constraints.h"

#include <set>
#include <vector>
#include <print>

namespace operator_counting {
    void FactoredConstraints::add_factor_lts_to_lp(const fts::LabelledTransitionSystem& lts, lp::LinearProgram& lp, int factor) {
        utils::unused_variable(factor);
        LPVariables& lp_variables = lp.get_variables();
        LPConstraints& lp_constraints = lp.get_constraints();

        // Add an init variable for each state in the LTS
        lts_init_variables.emplace_back();
        for (int s = 0; s < lts.size(); ++s) {
            lp_variables.emplace_back(lp::LPVariable(0., lp.get_infinity(), 0., false));
            int ivar = lp_variables.size() - 1;
#ifndef NDEBUG
            lp_variables.set_name(ivar, std::format("I[f={},{}]", factor, s));
#endif
            lts_init_variables.back().push_back(ivar);
        }

        // Add transition variables and collect ingoing/outgoing transitions for each state
        std::vector<std::set<int>> state_ingoing(lts.size(), std::set<int>());
        std::vector<std::set<int>> state_outgoing(lts.size(), std::set<int>());
        std::vector<std::set<int>> state_self_loops(lts.size(), std::set<int>());
        std::vector<std::vector<int>> label_group_transitions(lts.get_num_label_groups(), std::vector<int>());

        lts_transition_variables.emplace_back();

        for (const auto& [from, to, lg] : lts.get_transitions()) {

            // Add transition variable
            lp_variables.emplace_back(lp::LPVariable(0., lp.get_infinity(), 0., false));
            auto transition_variable = lp_variables.size() - 1;
#ifndef NDEBUG
            lp_variables.set_name(transition_variable, std::format("T[f={},{}-{}>{}]", factor, from, std::to_string(lg.group), to));
#endif
            lts_transition_variables.back().emplace_back(transition_variable);

            // Add this transition to the list of transitions for the label
            label_group_transitions.at(lg.group).push_back(transition_variable);

            if (from == to) {
                state_self_loops[from].insert(transition_variable);
            } else {
                // Add this transition to the list of ingoing/outgoing transitions for the target/source states
                state_ingoing[to].insert(transition_variable);
                state_outgoing[from].insert(transition_variable);
            }
        }

        // Add the flow constraint for each state
        for (int s = 0; s < lts.size(); ++s) {
            // 0 <= Sum of ingoing transitions - sum of outgoing transitions + initial state <= goal state? 1: 0
            lp::LPConstraint flow_constraint(0., lts.is_goal(s)? 1. : 0.);
            for (const auto& ingoing : state_ingoing[s]) {
                flow_constraint.insert(ingoing, 1.);
            }
            for (const auto& outgoing : state_outgoing[s]) {
                flow_constraint.insert(outgoing, -1.);
            }
            flow_constraint.insert(lts_init_variables.back().at(s), 1.);
            lp_constraints.push_back(flow_constraint);
        }

        // Add operator count constraints for each label group
        for (const auto& [lg_i, lg] : std::views::enumerate(lts.get_label_groups())) {
            lp::LPConstraint label_constraint(0., 0);
            for (const auto& label : lg) {
                label_constraint.insert(label, 1.);
            }
            for (const auto& transition : label_group_transitions.at(lg_i)) {
                label_constraint.insert(transition, -1.);
            }
            lp_constraints.push_back(label_constraint);
        }
    }

    void FactoredConstraints::initialize_constraints(const std::shared_ptr<AbstractTask>& task, lp::LinearProgram& lp) {
        fts::AtomicTaskFactory fts_factory;
        const auto transformed_task = fts_factory.transform_to_fts(task);

        // Create all the LP variables
        for (int i = 0; i < static_cast<int>(transformed_task.fts_task->get_factors().size()); ++i) {
            add_factor_lts_to_lp(transformed_task.fts_task->get_factor(i), lp, i);
        }
    }

    bool FactoredConstraints::update_constraints(const State& state, lp::LPSolver& lp_solver) {
        state.unpack();
        const auto& explicit_state = state.get_unpacked_values();

        LPConstraints lp_constraints;
        // Create factor constraints
        for (int i = 0; i < static_cast<int>(explicit_state.size()); ++i) {
            lp::LPConstraint constraint_i(1, lp_solver.get_infinity());
            constraint_i.insert(lts_init_variables.at(i).at(explicit_state.at(i)), 1.);
            lp_constraints.push_back(constraint_i);
        }

        lp_solver.add_temporary_constraints(lp_constraints);
        return false;
    }

    class FactoredConstraintsFeature : public plugins::TypedFeature<ConstraintGenerator, FactoredConstraints> {
    public:
        FactoredConstraintsFeature() : TypedFeature("factored_constraints") {
            document_title("Factored Abstraction Constraints");
            document_synopsis("");
        }

        [[nodiscard]] std::shared_ptr<FactoredConstraints> create_component(const plugins::Options &opts) const override {
            utils::unused_variable(opts);
            return std::make_shared<FactoredConstraints>();
        }
    };

    static plugins::FeaturePlugin<FactoredConstraintsFeature> _plugin;
}
#include "factored_constraints.h"
