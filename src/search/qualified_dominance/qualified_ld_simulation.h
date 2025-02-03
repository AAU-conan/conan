#ifndef DOMINANCE_LD_SIMULATION_H
#define DOMINANCE_LD_SIMULATION_H

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
#include "../dominance/factored_dominance_relation.h"

namespace fts {
class FTSTask;
}

namespace dominance {
class IncrementalLDSimulation : public DominanceAnalysis {
    utils::LogProxy log;
    std::shared_ptr<FactorDominanceRelationFactory> factor_dominance_relation_factory;

public:
    explicit IncrementalLDSimulation(utils::Verbosity verbosity, std::shared_ptr<FactorDominanceRelationFactory> factory)
        : log(utils::get_log_for_verbosity(verbosity)), factor_dominance_relation_factory(std::move(factory)) {
    };

    virtual ~IncrementalLDSimulation() = default;
    std::unique_ptr<FactoredDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) override;
    std::unique_ptr<FactoredDominanceRelation> compute_ld_simulation(const fts::FTSTask &task, utils::LogProxy &log);
};



    bool labels_simulate_labels(int factor, const std::unordered_set<int>& l1s, const std::vector<int>& l2s, bool include_noop, const LabelGroupedLabelRelation& label_relation);

    // Update the simulation relation for when t simulates s
    bool update_pairs(int factor, FactorDominanceRelation& local_relation, const LabelGroupedLabelRelation& label_relation);

    bool update_local_relation(int factor, FactorDominanceRelation& local_relation, const LabelGroupedLabelRelation& label_relation);
} // namespace dominance

#endif
