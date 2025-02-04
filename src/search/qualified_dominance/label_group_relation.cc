#include "label_group_relation.h"
#include "label_grouped_label_relation.h"
#include "../dominance/factored_dominance_relation.h"

namespace dominance {
bool LabelGroupSimulationRelation::compute_simulates(
    const LabelGroup lg1, const LabelGroup lg2,
    const FactorDominanceRelation &sim) const {
  // Check if lg1 simulates lg2 in lts
  // ∀s -lg2--> s'. ∃s -lg1-> s''. s'' simulates s'
  return std::ranges::all_of(
      lts.get_transitions_label_group(lg2), [&](const TSTransition &tr_2) {
        return std::ranges::any_of(targets_for_label_group_state(lg1, tr_2.src),
                                   [&](const int &tr_1_tgt) {
                                     return sim.simulates(tr_1_tgt,
                                                          tr_2.target);
                                   });
      });
}

bool LabelGroupSimulationRelation::compute_noop_simulates(
    const LabelGroup lg, const FactorDominanceRelation &sim) const {
  return std::ranges::all_of(
      lts.get_transitions_label_group(lg),
      [&](const auto &tr) { return sim.simulates(tr.src, tr.target); });
}

bool LabelGroupSimulationRelation::compute_simulates_noop(
    const LabelGroup lg, const FactorDominanceRelation &sim) const {
  // This assumes that lg has a transition in every state!
  return std::ranges::all_of(
      lts.get_transitions_label_group(lg),
      [&](const auto &tr) { return sim.simulates(tr.target, tr.src); });
}

bool LabelGroupSimulationRelation::update(const FactorDominanceRelation &sim) {
  bool changes = false;
  changes |= 0 < erase_if(simulations, [&](const std::pair<LabelGroup, LabelGroup> p) {
               return !compute_simulates(p.first, p.second, sim);
             });
  changes |= 0 < erase_if(noop_simulations, [&](const LabelGroup lg) {
               return !compute_noop_simulates(lg, sim);
             });
  changes |= 0 < erase_if(simulations_noop, [&](const LabelGroup lg) {
               return !compute_simulates_noop(lg, sim);
             });
#ifndef NDEBUG
  // std::println("{} simulations: {}", factor, boost::algorithm::join(std::views::transform(simulations, [&](const auto& p) { return std::format("{} <= {}", lts.label_group_name(p.second), lts.label_group_name(p.first));}) | std::ranges::to<std::vector>(), ", "));
  // std::println("{} simulations_noop: {}", factor, boost::algorithm::join(std::views::transform(simulations_noop, [&](const auto& p) { return std::format("{}", lts.label_group_name(p));}) | std::ranges::to<std::vector>(), ", "));
  // std::println("{} noop_simulations: {}", factor, boost::algorithm::join(std::views::transform(noop_simulations, [&](const auto& p) { return std::format("{}", lts.label_group_name(p));}) | std::ranges::to<std::vector>(), ", "));
#endif
  return changes;
}
} // namespace dominance
