
#include "contrastive_landmarks.h"
#include <set>

#include "../factored_transition_system/draw_fts.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../plugins/plugin.h"
#include "delete_relaxation_if_constraints.h"

namespace operator_counting {
void ContrastiveLandmarks::initialize_constraints(
    const std::shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) {
  utils::unused_variable(lp);
  fts::AtomicTaskFactory fts_factory;
  fts_task = fts_factory.transform_to_fts(task).fts_task;

#ifndef NDEBUG
  fts::draw_fts("fts.dot", *fts_task);
#endif
}

bool ContrastiveLandmarks::update_constraints_g_value(const State &state,
                                                      int g_value,
                                                      lp::LPSolver &lp_solver) {
  state.unpack();
  auto explicit_state = state.get_unpacked_values();

  if (std::ranges::all_of(std::views::enumerate(explicit_state), [&](auto iv) {
        const auto [i, v] = iv;
        return fts_task->get_factor(i).is_goal(v);
      })) {
    return false;
  }

  // for (int i = 0; i < explicit_state.size(); ++i) {
  //   std::cout << fts_task->get_fact_name({i, explicit_state[i]}) << ", ";
  // }
  // std::cout << std::endl;

  LPConstraints op_constraints;
  for (const auto &previous_state : previous_states) {
    if (previous_state.g_value <= g_value && std::ranges::all_of(std::views::iota(0ul, explicit_state.size()), [&](int i) { return !fts_task->get_factor(i).is_goal(explicit_state.at(i)) || fts_task->get_factor(i).is_goal(previous_state.state.at(i)); })) {
      lp::LPConstraint op_constraint(1., lp_solver.get_infinity());
      // std::cout << "Disjunctive landmark: ";
      std::set<int> used_labels;
      for (size_t i = 0; i < explicit_state.size(); ++i) {
        if (explicit_state.at(i) != previous_state.state.at(i)) {
          std::set<int> pstate_lgs =
              std::views::transform(
                  fts_task->get_factor(i).get_transitions(
                      previous_state.state[i]),
                  [](fts::LTSTransition tr) { return tr.label_group.group; }) |
              std::ranges::to<std::set<int>>();
          auto cstate_lgs =
              fts_task->get_factor(i).get_transitions(explicit_state[i]) |
              std::views::transform(
                  [](fts::LTSTransition tr) { return tr.label_group.group; }) |
              std::views::filter(
                  [&](int lg) { return !pstate_lgs.contains(lg); });
          for (const int &lg : cstate_lgs) {
            for (const int &label :
                 fts_task->get_factor(i).get_labels(fts::LabelGroup(lg))) {
              if (!used_labels.contains(label)) {
                op_constraint.insert(label, 1.);
                //std::cout << fts_task->get_operator_name(label, false) << ",";
                used_labels.insert(label);
              }
            }
          }
        }
      }
//      std::cout << std::endl;

      op_constraints.push_back(op_constraint);
    }
  }
  lp_solver.add_temporary_constraints(op_constraints);

  previous_states.emplace_back(explicit_state, g_value);
  return false;
}

class ContrastiveLandmarksConstraintsFeature
    : public plugins::TypedFeature<ConstraintGenerator, ContrastiveLandmarks> {
public:
  ContrastiveLandmarksConstraintsFeature()
      : TypedFeature("contrastive_landmarks") {
    document_title("Contrastive Landmark constraints");
    document_synopsis("");
  }

  [[nodiscard]] std::shared_ptr<ContrastiveLandmarks>
  create_component(const plugins::Options &,
                   const utils::Context &) const override {
    return std::make_shared<ContrastiveLandmarks>();
  }
};

static plugins::FeaturePlugin<ContrastiveLandmarksConstraintsFeature> _plugin;
} // namespace operator_counting
