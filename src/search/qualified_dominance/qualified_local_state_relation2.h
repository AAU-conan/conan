#ifndef QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H
#define QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H

#include <string>
#include <unordered_set>
#include <vector>

#include "../factored_transition_system/fact_names.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "qualified_label_relation.h"
#include <mata/nfa/nfa.hh>
#include <mata/nfa/types.hh>
#include <rust/cxx.h>

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

namespace qdominance {
using TransitionIndex = size_t;
TransitionIndex constexpr NOOP_TRANSITION = -1;

// First implementation of a simulation relation.
class QualifiedLocalStateRelation2 {
protected:
  LabelledTransitionSystem lts;
  int factor;
  const FTSTask &fts_task;

public:
  // For each state, the set of states that it simulates
  std::unordered_set<std::pair<int, int>> simulations;

  // Vectors of states dominated/dominating by each state. Lazily computed when
  // needed.
  std::vector<std::vector<int>> dominated_states, dominating_states;
  void compute_list_dominated_states();

  void cancel_simulation_computation();

  bool update(const QualifiedLabelRelation &label_relation);
  bool noop_simulates_tr(int t, LTSTransition s_tr,
                         const QualifiedLabelRelation &label_relation) const;
  bool tr_simulates_tr(const LTSTransition &t_tr, const LTSTransition &s_tr,
                       const QualifiedLabelRelation &label_relation) const;
  bool labels_simulate_labels(const std::set<int> &l1s,
                              const std::vector<int> &l2s, bool include_noop,
                              const QualifiedLabelRelation &label_relation);
  bool update_pairs(const QualifiedLabelRelation &label_relation);
  QualifiedLocalStateRelation2(const LabelledTransitionSystem &lts, int factor,
                               const FTSTask &fts_task);

  void print_simulations() const {
    for (auto &sim : simulations) {
      if (!lts.get_transitions(sim.first).empty() &&
          !lts.get_transitions(sim.second).empty()) {
        std::println("    {} <= {}", lts.state_name(sim.second),
                     lts.state_name(sim.first));
      }
    }
  }

  [[nodiscard]] bool simulates(int t, int s) const {
    return s == t || simulations.contains({t, s});
  }

  [[nodiscard]] bool similar(int s, int t) const {
    return simulates(s, t) && simulates(t, s);
  }

  [[nodiscard]] int num_states() const { return lts.size(); }

  bool is_identity() const;
  int num_equivalences() const;
  int num_simulations(bool ignore_equivalences) const;
  int num_different_states() const;

  void dump(utils::LogProxy &log, const std::vector<std::string> &names) const;
  void dump(utils::LogProxy &log) const;

  // Computes the probability of selecting a random pair s, s' such that s
  // simulates s'.
  double get_percentage_simulations(bool ignore_equivalences) const;

  // Computes the probability of selecting a random pair s, s' such that s is
  // equivalent to s'.
  double get_percentage_equivalences() const;
};
} // namespace qdominance
#endif
