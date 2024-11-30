#ifndef DOMINANCE_DOMINANCE_PRUNING_PREVIOUS_H
#define DOMINANCE_DOMINANCE_PRUNING_PREVIOUS_H

#include "dominance_pruning.h"
#include "../task_proxy.h"
#include <unordered_set>
#include <map>

#include "dominance_database.h"


namespace dominance {
    class DominancePruningPrevious : public DominancePruning {
    public:
        std::shared_ptr<DominanceDatabaseFactory> database_factory;
        std::shared_ptr<DominanceDatabase> database;

        DominancePruningPrevious(std::shared_ptr<DominanceDatabaseFactory> database_factory,
                                const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                utils::Verbosity verbosity);

        virtual ~DominancePruningPrevious() = default;

        virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
        virtual void prune_generation(const State &state, const SearchNodeInfo &, std::vector<OperatorID> &op_ids) override;
        virtual bool must_prune_operator(const OperatorProxy & op,
                                 const State & state,
                                 const ExplicitState & parent,
                                 const ExplicitState & parent_transformed,
                                 ExplicitState & succ,
                                 ExplicitState & succ_transformed,
                                 int g_value);
    };
}

#endif

// conan-downward ../../domain.pddl ../../p01.pddl --search "astar(blind(), pruning=dominance_all_previous())"
