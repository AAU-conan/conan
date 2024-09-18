#ifndef DOMINANCE_DOMINANCE_PRUNING_LOWER_G_H
#define DOMINANCE_DOMINANCE_PRUNING_LOWER_G_H

#include "dominance_pruning.h"
#include "../task_proxy.h"
#include <unordered_set>
#include <map>

typedef std::vector<int> ExplicitState;

namespace dominance {

    class DominancePruningLowerG : public DominancePruning {

        // Store previously generated states
        std::vector<ExplicitState> previous_states;
        std::vector<ExplicitState> previous_transformed_states;

        // The key is the g_value, and the value is a vector<vector<int>> with the transformed states with that value
        std::map<int, std::vector<std::vector<int>>> previous_states_sorted;


        bool must_prune_operator(const OperatorProxy & op,
                                 const State & state,
                                 const ExplicitState & parent,
                                 const ExplicitState & parent_transformed,
                                 ExplicitState & succ,
                                 ExplicitState & succ_transformed,
                                 int g_value) ;

        bool compare_against_all_previous_states(const ExplicitState &succ_transformed, int g_value) const;

        void store_state(const ExplicitState &transformed_state, int g_value);

    public:
        DominancePruningLowerG(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                    std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                    utils::Verbosity verbosity);
        virtual ~DominancePruningLowerG() = default;

        virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
        virtual void prune_generation(const State &state, const SearchNodeInfo & , std::vector<OperatorID> &op_ids) override;

        struct StateInfo {
            std::vector<int> transformed_state;
            int g_value;

            StateInfo(const std::vector<int>& ts, int g)
                : transformed_state(ts), g_value(g) {}
        };




    };
}

#endif
// conan-downward ../../domain.pddl ../../p01.pddl --search "astar(blind(), pruning=dominance_all_previous())"
