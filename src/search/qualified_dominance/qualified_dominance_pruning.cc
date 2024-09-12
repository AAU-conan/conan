#include "qualified_dominance_pruning.h"

#include <cassert>
#include <vector>
#include <limits>
#include "../plugins/options.h"
#include "../plugins/plugin.h"
#include "../factored_transition_system/fts_task.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../factored_transition_system/factored_state_mapping.h"
#include "../factored_transition_system/draw_fts.h"

#include <rust/cxx.h>
#include <rust_test_cxx/lib.h>
#include <rust_test_cxx/ltl.h>

using namespace std;
using plugins::Options;

namespace qdominance {

    QualifiedDominancePruning::QualifiedDominancePruning(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                       std::shared_ptr<QualifiedDominanceAnalysis> dominance_analysis,
                                       utils::Verbosity verbosity) :
    PruningMethod(verbosity), fts_factory(fts_factory), dominance_analysis(dominance_analysis) {
    }

    void QualifiedDominancePruning::initialize(const std::shared_ptr<AbstractTask> &task) {
// TODO (documentation):        dump_options();
        PruningMethod::initialize(task);

        fts::TransformedFTSTask transformed_task = fts_factory->transform_to_fts(task);
        state_mapping = std::move(transformed_task.factored_state_mapping);

        std::cout << "drawing graph" << std::endl;
        fts::draw_fts("fts.dot", *transformed_task.fts_task);

        dominance_relation = dominance_analysis->compute_dominance_relation(*transformed_task.fts_task);

        if (log.is_at_least_verbose()){
            dominance_relation->dump_statistics(log);
        }
    }

    void add_dominance_pruning_options_to_feature(plugins::Feature &feature) {
        feature.add_option<shared_ptr<fts::FTSTaskFactory>>(
                "fts_factory",
                "Strategy to construct a FTS representation of the planning task ",
                "atomic()");

        feature.add_option<shared_ptr<QualifiedDominanceAnalysis>>(
                "quantified_dominance_analysis",
                "Strategy to obtain a dominance relation",
                "qld_simulation()");

        add_pruning_options_to_feature(feature);
    }

    tuple<std::shared_ptr<fts::FTSTaskFactory>, std::shared_ptr<QualifiedDominanceAnalysis>, utils::Verbosity> get_dominance_pruning_arguments_from_options(const plugins::Options &opts) {
        return tuple_cat(make_tuple(opts.get<std::shared_ptr<fts::FTSTaskFactory>>("fts_factory"),
                                    opts.get<std::shared_ptr<QualifiedDominanceAnalysis>>("quantified_dominance_analysis")),
                         get_pruning_arguments_from_options(opts));
    }

}