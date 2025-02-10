#ifndef DOMINANCE_INCREMENTAL_LD_SIMULATION_H
#define DOMINANCE_INCREMENTAL_LD_SIMULATION_H

#include "../dominance/all_none_factor_index.h"
#include "../factored_transition_system/fts_task.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../utils/logging.h"
#include "sparse_local_state_relation.h"
#include <fstream>
#include <ostream>
#include <vector>

#include "../utils/logging.h"
#include "../utils/timer.h"
#include "../dominance/dominance_analysis.h"
#include "../dominance/state_dominance_relation.h"
#include "../dominance/ld_simulation.h"

namespace fts {
class FTSTask;
}

namespace dominance {
/*
 * A version of Label Dominance simulation that only updates the factors in the label relation that were changed by the
 * last iteration of the state relation.
 */
class IncrementalLDSimulation : public DominanceAnalysis {
    utils::LogProxy log;
    std::shared_ptr<FactorDominanceRelationFactory> factor_dominance_relation_factory;
        std::shared_ptr<LabelRelationFactory> label_relation_factory;

public:
    explicit IncrementalLDSimulation(utils::Verbosity verbosity, std::shared_ptr<FactorDominanceRelationFactory> factor_dominance_relation_factory, std::shared_ptr<LabelRelationFactory> label_relation_factory)
        : log(utils::get_log_for_verbosity(verbosity)), factor_dominance_relation_factory(std::move(factor_dominance_relation_factory)), label_relation_factory(std::move(label_relation_factory)) {
    };

    ~IncrementalLDSimulation() override = default;
    std::unique_ptr<StateDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) override;
    std::unique_ptr<StateDominanceRelation> compute_ld_simulation(const fts::FTSTask &task, utils::LogProxy &log);
};

    bool labels_simulate_labels(int factor, const std::unordered_set<int>& l1s, const std::vector<int>& l2s, bool include_noop, const LabelRelation& label_relation);

    // Update the simulation relation for when t simulates s
    bool update_pairs(int factor, FactorDominanceRelation& local_relation, const LabelRelation& label_relation);

} // namespace dominance

#endif
