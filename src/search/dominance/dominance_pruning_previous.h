#ifndef DOMINANCE_DOMINANCE_PRUNING_PREVIOUS_H
#define DOMINANCE_DOMINANCE_PRUNING_PREVIOUS_H

#include "dominance_pruning.h"
#include "../task_proxy.h"
#include <unordered_set>
#include <map>

typedef std::vector<int> ExplicitState;

namespace dominance {

    class DominancePruningPrevious : public DominancePruning {

    protected:

        // Pure virtual methods to be implemented by derived classes
        virtual bool check(const ExplicitState &state, int g) const = 0;
        virtual void insert(const ExplicitState &transformed_state, int g) = 0;
        

    public:
        std::vector<ExplicitState> previous_transformed_states;

        DominancePruningPrevious(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
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


    class DominancePruningAllPrevious : public DominancePruningPrevious {

        // Store known states
        std::vector<ExplicitState> previous_transformed_states;

        //Methods to keep dominated states in explicit search
        //Check: returns true if a better or equal state is known
        bool check(const ExplicitState &state, int g) const override;
        //Insert: inserts a state into the set of known states
        void insert(const ExplicitState &transformed_state, int g) override;

    public:
        DominancePruningAllPrevious(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                    std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                    utils::Verbosity verbosity);

        virtual ~DominancePruningAllPrevious() = default;
    };



    class DominancePruningPreviousLowerG : public DominancePruningPrevious {

        // Store known states
        std::vector<ExplicitState> previous_transformed_states;
        // key: g_value; value: vector<vector<int>> with the transformed states with that g_value
        std::map<int, std::vector<std::vector<int>>> previous_states_sorted;

        //Methods to keep dominated states in explicit search
        //Check: returns true if a better or equal state is known
        bool check(const ExplicitState &succ_transformed, int g_value) const override;
        //Insert: inserts a state into the set of known states
        void insert(const ExplicitState &transformed_state, int g_value) override;

    public:
        DominancePruningPreviousLowerG(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                    std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                    utils::Verbosity verbosity);
                                    
        virtual ~DominancePruningPreviousLowerG() = default;
    };
}


#endif

// conan-downward ../../domain.pddl ../../p01.pddl --search "astar(blind(), pruning=dominance_all_previous())"
