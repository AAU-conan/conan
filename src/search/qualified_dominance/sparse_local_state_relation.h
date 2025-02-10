#ifndef SPARSE_LOCAL_STATE_RELATION_H
#define SPARSE_LOCAL_STATE_RELATION_H

#include <string>
#include <unordered_set>
#include <vector>

#include "../factored_transition_system/fact_names.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "label_grouped_label_relation.h"
#include "../dominance/local_state_relation.h"
#include <mata/nfa/nfa.hh>


namespace merge_and_shrink {
class TransitionSystem;
}

namespace fts {
class FTSTask;
class LabelledTransitionSystem;
} // namespace fts

namespace utils {
class LogProxy;
}

namespace dominance {
using TransitionIndex = size_t;
TransitionIndex constexpr NOOP_TRANSITION = -1;

// First implementation of a simulation relation.
class SparseLocalStateRelation final : public FactorDominanceRelation {
public:
  // For each state, the set of states that it simulates
  std::unordered_set<std::pair<int, int>> simulations;

  void cancel_simulation_computation() override;

  explicit SparseLocalStateRelation(const LabelledTransitionSystem &lts);

  void print_simulations() const;

  [[nodiscard]] bool simulates(int t, int s) const override;

  [[nodiscard]] bool similar(int s, int t) const override;

  [[nodiscard]] int num_states() const { return lts.size(); }

  int num_simulations() const override;
  bool applySimulations(std::function<bool(int s, int t)>&& f) const override;
  bool removeSimulations(std::function<bool(int s, int t)>&& f) override;
};
} // namespace dominance
#endif
