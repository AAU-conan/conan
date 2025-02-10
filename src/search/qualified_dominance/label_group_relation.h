#ifndef LABEL_GROUP_RELATION_H
#define LABEL_GROUP_RELATION_H

#include <print>

#include "../factored_transition_system/fts_task.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include <mata/utils/utils.hh>

using namespace fts;

namespace dominance {
class FactorDominanceRelation;
class LabelGroupedLabelRelation;
} // namespace dominance

namespace dominance {
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
                                        int factor);

  [[nodiscard]] bool simulates(const LabelGroup lg1,
                               const LabelGroup lg2) const;

  // Computes whether lg is simulated by noop in all other transition systems
  [[nodiscard]] bool noop_simulates(const LabelGroup lg) const;

  [[nodiscard]] const std::vector<int> &
  targets_for_label_group_state(LabelGroup lg, int s) const;

  [[nodiscard]] bool
  compute_simulates(LabelGroup lg1, LabelGroup lg2,
                    const FactorDominanceRelation &sim) const;
  bool compute_simulates_noop(LabelGroup lg,
                              const FactorDominanceRelation &sim) const;
  [[nodiscard]] bool
  compute_noop_simulates(LabelGroup lg,
                         const FactorDominanceRelation &sim) const;
  bool update(const FactorDominanceRelation &sim);
};
} // namespace dominance

#endif // LABEL_GROUP_RELATION_H
