#ifndef DOMINANCE_DOMINANCE_PRUNING_H
#define DOMINANCE_DOMINANCE_PRUNING_H

#include <vector>
#include <iostream>
#include <memory>
#include <set>

#include "../pruning_method.h"
#include "factored_dominance_relation.h"
#include "dominance_analysis.h"

namespace plugins {
    class Options;
}

namespace fts {
    class FactoredStateMapping;
    class FTSTaskFactory;
}

namespace dominance {

    class  DominancePruning : public PruningMethod {
    protected:
        std::shared_ptr<fts::FTSTaskFactory> fts_factory;
        std::shared_ptr<DominanceAnalysis> dominance_analysis;

        //TODO: This will be separated on a TaskDependentPruningMethod when the refactoring from FastDownward is completed
        std::unique_ptr<FactoredDominanceRelation> dominance_relation;
        std::shared_ptr<fts::FactoredStateMapping> state_mapping;

        void dump_options() const;

    public:
        DominancePruning(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                         std::shared_ptr<DominanceAnalysis> dominance_analysis,
                         utils::Verbosity verbosity);

        virtual ~DominancePruning() = default;
        virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
      //  virtual void print_statistics() const override;
    };

    extern void add_dominance_pruning_options_to_feature(plugins::Feature &feature);
    extern std::tuple<std::shared_ptr<fts::FTSTaskFactory>,std::shared_ptr<DominanceAnalysis>, utils::Verbosity> get_dominance_pruning_arguments_from_options(
            const plugins::Options &opts);

}
#endif
