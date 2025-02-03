#include "dominance_pruning.h"

#include "../plugins/options.h"
#include "../plugins/plugin.h"

#include "../factored_transition_system/fts_task.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../factored_transition_system/factored_state_mapping.h"

using namespace std;
using plugins::Options;

namespace dominance {

    DominancePruning::DominancePruning(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                       std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                       utils::Verbosity verbosity) :
    PruningMethod(verbosity), transformed_task(nullptr, nullptr), fts_factory(fts_factory), dominance_analysis(dominance_analysis) {
    }

    void DominancePruning::initialize(const std::shared_ptr<AbstractTask> &task) {
// TODO (documentation):        dump_options();
        PruningMethod::initialize(task);

        transformed_task = fts_factory->transform_to_fts(task);
        state_mapping = std::move(transformed_task.factored_state_mapping);

        dominance_relation = dominance_analysis->compute_dominance_relation(*transformed_task.fts_task);

        if (log.is_at_least_verbose()){
            dominance_relation->dump_statistics(log);
        }


#ifndef NDEBUG
        for (const auto& [i, rel] : std::views::enumerate(dominance_relation->get_local_relations())) {
            rel->dump(log);
        }
#endif
    }

    void add_dominance_pruning_options_to_feature(plugins::Feature &feature) {
        feature.add_option<shared_ptr<fts::FTSTaskFactory>>(
                "fts_factory",
                "Strategy to construct a FTS representation of the planning task ",
                "atomic()");

        feature.add_option<shared_ptr<DominanceAnalysis>>(
                "dominance_analysis",
                "Strategy to obtain a dominance relation",
                "ld_simulation()");

        add_pruning_options_to_feature(feature);
    }

    tuple<std::shared_ptr<fts::FTSTaskFactory>, std::shared_ptr<DominanceAnalysis>, utils::Verbosity> get_dominance_pruning_arguments_from_options(const plugins::Options &opts) {
        return tuple_cat(make_tuple(opts.get<std::shared_ptr<fts::FTSTaskFactory>>("fts_factory"),
                                    opts.get<std::shared_ptr<DominanceAnalysis>>("dominance_analysis")),
                         get_pruning_arguments_from_options(opts));
    }

}