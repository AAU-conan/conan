#ifndef DOMINANCE_INCREMENTAL_LD_SIMULATION_H
#define DOMINANCE_INCREMENTAL_LD_SIMULATION_H

#include <fstream>
#include <vector>
#include <unordered_set>

#include "dominance_analysis.h"
#include "../utils/logging.h"

namespace fts {
class FTSTask;
}

namespace dominance {
    class FactorDominanceRelation;
    class LabelRelation;
    class LabelRelationFactory;
    class FactorDominanceRelationFactory;
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

    bool labels_simulate_labels(int factor, const fts::FTSTask& task, const std::unordered_set<int>& l1s, const std::vector<int>& l2s, bool include_noop, const LabelRelation& label_relation);

    // Update the simulation relation for when t simulates s
    bool update_pairs(int factor, FactorDominanceRelation& local_relation, const LabelRelation& label_relation);

} // namespace dominance

#endif
