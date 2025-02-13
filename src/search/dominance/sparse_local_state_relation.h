#ifndef SPARSE_LOCAL_STATE_RELATION_H
#define SPARSE_LOCAL_STATE_RELATION_H

#include <unordered_set>

#include "factor_dominance_relation.h"

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

  explicit SparseLocalStateRelation(const fts::LabelledTransitionSystem &lts);

  void print_simulations() const;

  [[nodiscard]] bool simulates(int t, int s) const override;

  [[nodiscard]] bool similar(int s, int t) const override;

  int num_simulations() const override;
  bool apply_to_simulations_until(std::function<bool(int s, int t)>&& f) const override;
  bool remove_simulations_if(std::function<bool(int s, int t)>&& f) override;
};
} // namespace dominance
#endif
