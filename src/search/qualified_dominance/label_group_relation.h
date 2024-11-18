#ifndef LABEL_GROUP_RELATION_H
#define LABEL_GROUP_RELATION_H

#include <print>

#include "../factored_transition_system/fts_task.h"
#include <mata/utils/utils.hh>

using namespace fts;

namespace qdominance {
class QualifiedLocalStateRelation2;
class QualifiedLabelRelation;
} // namespace qdominance

namespace qdominance {
class LabelGroupSimulationRelation {
  // For lg, state s, the states s' s.t. s -lg-> s'
  std::vector<std::vector<std::vector<int>>> label_group_state_targets;

public:
  const LabelledTransitionSystem &lts;
  int factor;
  
  // The pairs (lg1, lg2) where lg1 simulates lg2, and both lg1 and lg2 are
  // relevant
  std::unordered_set<std::pair<LabelGroup, LabelGroup>> simulations;

  // The label groups that are simulated by noop
  std::unordered_set<LabelGroup> noop_simulations;

  // The label groups that simulate noop
  std::unordered_set<LabelGroup> simulations_noop;

  explicit LabelGroupSimulationRelation(const LabelledTransitionSystem &lts,
                                        int factor)
      : lts(lts), factor(factor) {
    const auto all_labels_groups =
        std::views::iota(0, lts.get_num_label_groups()) |
        std::views::transform([](auto lg) { return LabelGroup(lg); }) |
        std::ranges::to<std::unordered_set>();
   
    std::unordered_map<int, std::set<int>> lg_sources;
    for (const auto &lg : lts.get_relevant_label_groups()) {
      lg_sources.emplace(
          lg.group,
          lts.get_transitions_label_group(lg) |
              std::views::transform([](const auto &tr) { return tr.src; }) |
              std::ranges::to<std::set>());
    }

    // lg1 simulates lg2 only if lg2 can be applied whenever lg1 can
    for (const auto &lg1 : lts.get_relevant_label_groups()) {
      for (const auto &lg2 : lts.get_relevant_label_groups()) {
        if (std::ranges::includes(lg_sources.at(lg2.group),
                                  lg_sources.at(lg1.group))) {
          simulations.insert({lg1, lg2});
        }
      }
    }

    // lg simulates noop only if it has a transition in every state
    for (int lg = 0; lg < lts.get_num_label_groups(); ++lg) {
      if (!lg_sources.contains(lg) || lg_sources.at(lg).size() == lts.size()) {
        // If lg is not in lg_sources, it is irrelevant
        simulations_noop.insert(LabelGroup(lg));
      }
    }

    // Noop can always be applied
    noop_simulations = all_labels_groups;

    label_group_state_targets.resize(lts.get_num_label_groups(),
                                     std::vector<std::vector<int>>(lts.size()));
    for (const auto tr : lts.get_transitions()) {
      label_group_state_targets.at(tr.label_group.group)
          .at(tr.src)
          .push_back(tr.target);
    }
  }

  [[nodiscard]] bool simulates(const LabelGroup lg1,
                               const LabelGroup lg2) const {
    if (lts.is_relevant_label_group(lg1)) {
      if (lts.is_relevant_label_group(lg2)) {
        return simulations.contains({lg1, lg2});
      } else {
        return simulations_noop.contains(lg1);
      }
    } else {
      return noop_simulations.contains(lg2);
    }
  }

  // Computes whether lg is simulated by noop in all other transition systems
  [[nodiscard]] bool noop_simulates(const LabelGroup lg) const {
    return noop_simulations.contains(lg);
  }

  [[nodiscard]] const std::vector<int> &
  targets_for_label_group_state(LabelGroup lg, int s) const {
    return label_group_state_targets.at(lg.group).at(s);
  }

  [[nodiscard]] bool
  compute_simulates(LabelGroup lg1, LabelGroup lg2,
                    const QualifiedLocalStateRelation2 &sim) const;
  bool compute_simulates_noop(LabelGroup lg,
                              const QualifiedLocalStateRelation2 &sim) const;
  [[nodiscard]] bool
  compute_noop_simulates(LabelGroup lg,
                         const QualifiedLocalStateRelation2 &sim) const;
  bool update(const QualifiedLocalStateRelation2 &sim);
};
} // namespace qdominance

#endif // LABEL_GROUP_RELATION_H
