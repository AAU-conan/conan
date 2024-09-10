#ifndef DOMINANCE_DOMINANCE_PRUNING_LOCAL_H
#define DOMINANCE_DOMINANCE_PRUNING_LOCAL_H

#include "qualified_dominance_pruning.h"
#include "../task_proxy.h"

namespace qdominance {
    class QualifiedDominancePruningLocal : public QualifiedDominancePruning {
        const bool compare_initial_state;
        const bool compare_siblings;


        bool must_prune_operator(const OperatorProxy & op,
                                 const State & state,
                                 const std::vector<int> & parent,
                                 const std::vector<int> & parent_transformed,
                                 std::vector<int> & succ,
                                 std::vector<int> & succ_transformed) const;

    public:
        QualifiedDominancePruningLocal(bool compare_initial_state, bool compare_siblings,
                              const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                              std::shared_ptr<QualifiedDominanceAnalysis> dominance_analysis,
                              utils::Verbosity verbosity);
        virtual ~QualifiedDominancePruningLocal() = default;

        virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
        virtual void prune(const State &state, std::vector<OperatorID> &op_ids) override;
    };
}


#endif
