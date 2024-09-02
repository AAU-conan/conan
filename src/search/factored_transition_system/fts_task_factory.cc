#include "fts_task_factory.h"

#include "fts_task.h"
#include "factored_state_mapping.h"
#include "labelled_transition_system.h"
#include "../merge_and_shrink/fts_factory.h"
#include "../merge_and_shrink/factored_transition_system.h"

#include "../plugins/plugin.h"

namespace fts {

    TransformedFTSTask::TransformedFTSTask(std::unique_ptr<FTSTask> &&fts_task,
                                           std::unique_ptr<FactoredStateMapping> &&factored_state_mapping) : fts_task(std::move(fts_task)),
                                                                                                             factored_state_mapping(std::move(factored_state_mapping)) {

    }

    static class FTSTaskFactoryCategoryPlugin : public plugins::TypedCategoryPlugin<FTSTaskFactory> {
    public:
        FTSTaskFactoryCategoryPlugin() : TypedCategoryPlugin("FTSTaskFactory") {
            document_synopsis(
                    "This page describes the various factories to construct Factored Transition Systems from Abstract Tasks."
            );
        }
    } _category_plugin;

    TransformedFTSTask AtomicTaskFactory::transform_to_fts(const std::shared_ptr<AbstractTask> &task) {
        TaskProxy task_proxy (*task);
        merge_and_shrink::FactoredTransitionSystem fts =
                merge_and_shrink::create_factored_transition_system(
                        task_proxy,
                        true,
                        true,
                        utils::g_log);

        return {std::make_unique<FTSTask>(fts), std::make_unique<FactoredStateMappingIdentity>()};
    }

    class AtomicTaskFactoryFeature
            : public plugins::TypedFeature<FTSTaskFactory, AtomicTaskFactory> {
    public:
        AtomicTaskFactoryFeature() : TypedFeature("atomic") {
            document_title("Constructs atomic transition systems");
        }

        virtual std::shared_ptr<AtomicTaskFactory> create_component(
                const plugins::Options &,
                const utils::Context &) const override {
            return plugins::make_shared_from_arg_tuples<AtomicTaskFactory>();
        }
    };



static plugins::FeaturePlugin<AtomicTaskFactoryFeature> _plugin_atomic;

    //TODO: Plugin and implementation for MergeAndShrinkTaskFactory
}