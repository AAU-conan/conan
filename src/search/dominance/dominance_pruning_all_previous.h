#ifndef DOMINANCE_DOMINANCE_PRUNING_ALL_PREVIOUS_H
#define DOMINANCE_DOMINANCE_PRUNING_ALL_PREVIOUS_H

#include "dominance_pruning.h"
#include "../task_proxy.h"
#include <unordered_set>

typedef std::vector<int> ExplicitState;

namespace dominance {
    class DominancePruningAllPrevious : public DominancePruning {

        // Store previously generated states
        std::vector<ExplicitState> previous_states;
        std::vector<ExplicitState> previous_transformed_states;

        bool must_prune_operator(const OperatorProxy & op,
                                 const State & state,
                                 const ExplicitState & parent,
                                 const ExplicitState & parent_transformed,
                                 ExplicitState & succ,
                                 ExplicitState & succ_transformed) ;

        bool compare_against_all_previous_states(const ExplicitState &succ_transformed) const;

        void store_state(const ExplicitState &transformed_state);

    public:
        DominancePruningAllPrevious(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                    std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                    utils::Verbosity verbosity);
        virtual ~DominancePruningAllPrevious() = default;

        virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
        virtual void prune_generation(const State &state, const SearchNodeInfo & , std::vector<OperatorID> &op_ids) override;
    };
}

#endif
// conan-downward ../../domain.pddl ../../p01.pddl --search "astar(blind(), pruning=dominance_all_previous())"
