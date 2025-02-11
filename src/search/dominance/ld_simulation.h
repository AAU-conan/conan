#ifndef DOMINANCE_LD_SIMULATION_H
#define DOMINANCE_LD_SIMULATION_H

#include <vector>
#include <ostream>
#include "state_dominance_relation.h"
#include "factor_dominance_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../utils/logging.h"

#include "../utils/timer.h"
#include "dominance_analysis.h"

namespace fts {
    class FTSTask;
    class LabelledTransitionSystem;
}

namespace dominance {
    class LabelRelation;
    class LabelRelationFactory;

    bool update_local_relation(int lts_id, const fts::LabelledTransitionSystem& lts, const LabelRelation& label_dominance,
                               FactorDominanceRelation& local_relation);
    bool update_label_relation(LabelRelation& label_relation, const fts::FTSTask & task, const std::vector<std::unique_ptr<FactorDominanceRelation>> &sim);

    class LDSimulation : public DominanceAnalysis {
        utils::LogProxy log;
        std::shared_ptr<FactorDominanceRelationFactory> factor_dominance_relation_factory;
        std::shared_ptr<LabelRelationFactory> label_relation_factory;

        std::unique_ptr<StateDominanceRelation> compute_ld_simulation(const fts::FTSTask & task, utils::LogProxy & log);
    public:
        explicit LDSimulation(utils::Verbosity verbosity, std::shared_ptr<FactorDominanceRelationFactory> factor_dominance_relation_factory, std::shared_ptr<LabelRelationFactory> label_relation_factory);

        virtual ~LDSimulation() = default;
        std::unique_ptr<StateDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) override;
    };
}

#endif
