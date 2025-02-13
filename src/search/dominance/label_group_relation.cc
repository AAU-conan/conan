#include "label_group_relation.h"
#include "label_grouped_label_relation.h"
#include "state_dominance_relation.h"

namespace dominance {
  LabelGroupSimulationRelation::LabelGroupSimulationRelation(const LabelledTransitionSystem& lts, int factor): lts(lts), factor(factor), noop_simulations(), simulations_noop() {
    const auto all_labels_groups =
      std::views::iota(0, lts.get_num_label_groups()) |
      std::views::transform([](auto lg) { return LabelGroup(lg); }) |
      std::ranges::to<std::unordered_set>();

    std::unordered_map<int, std::set<int>> lg_sources;
    for (const auto& lg : lts.get_relevant_label_groups()) {
      lg_sources.emplace(
        lg.group,
        lts.get_transitions_label_group(lg) |
        std::views::transform([](const auto& tr) { return tr.src; }) |
        std::ranges::to<std::set>());
    }

    // lg1 simulates lg2 only if lg2 can be applied whenever lg1 can
    for (const auto& lg1 : lts.get_relevant_label_groups()) {
      for (const auto& lg2 : lts.get_relevant_label_groups()) {
        if (std::ranges::includes(lg_sources.at(lg2.group),
                                  lg_sources.at(lg1.group))) {
          simulations.insert({lg2, lg1});
        }
      }
    }

    // lg simulates noop only if it has a transition in every state
    for (int lg = 0; lg < lts.get_num_label_groups(); ++lg) {
      if (!lg_sources.contains(lg) || lg_sources.at(lg).size() == static_cast<size_t>(lts.size())) {
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

#ifndef NDEBUG
    // std::println("{} simulations: {}", factor, boost::algorithm::join(std::views::transform(simulations, [&](const auto& p) { return std::format("{} <= {}", lts.label_group_name(p.second), lts.label_group_name(p.first));}) | std::ranges::to<std::vector>(), ", "));
    // std::println("{} simulations_noop: {}", factor, boost::algorithm::join(std::views::transform(simulations_noop, [&](const auto& p) { return std::format("{}", lts.label_group_name(p));}) | std::ranges::to<std::vector>(), ", "));
    // std::println("{} noop_simulations: {}", factor, boost::algorithm::join(std::views::transform(noop_simulations, [&](const auto& p) { return std::format("{}", lts.label_group_name(p));}) | std::ranges::to<std::vector>(), ", "));
#endif
  }

  bool LabelGroupSimulationRelation::simulates(const LabelGroup lg1, const LabelGroup lg2) const {
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

  bool LabelGroupSimulationRelation::noop_simulates(const LabelGroup lg) const {
    return noop_simulations.contains(lg);
  }

  const std::vector<int>& LabelGroupSimulationRelation::targets_for_label_group_state(LabelGroup lg, int s) const {
    return label_group_state_targets.at(lg.group).at(s);
  }

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
