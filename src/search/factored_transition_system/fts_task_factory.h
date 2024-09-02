#ifndef FTS_FTS_TASK_FACTORY_H
#define FTS_FTS_TASK_FACTORY_H

#include <memory>
#include "fts_task.h"
#include "factored_state_mapping.h"
class AbstractTask;

namespace fts {
    // Class to be used as return type
    struct TransformedFTSTask {
        std::unique_ptr<FTSTask> fts_task;
        std::unique_ptr<FactoredStateMapping> factored_state_mapping;

        TransformedFTSTask(std::unique_ptr<FTSTask> && fts_task,
                           std::unique_ptr<FactoredStateMapping> && factored_state_mapping);

    };

    class FTSTaskFactory {
    protected:
        virtual ~FTSTaskFactory() = default;
    public:
        virtual TransformedFTSTask transform_to_fts(const std::shared_ptr<AbstractTask> & task) = 0;
    };

    class AtomicTaskFactory : public FTSTaskFactory {
    public:
        virtual TransformedFTSTask transform_to_fts(const std::shared_ptr<AbstractTask> & task) override;
    };

/* TODO:    class MergeAndShrinkTaskFactory : public FTSTaskFactory {
    public:
        virtual TransformedFTSTask transform_to_fts(const std::shared_ptr<AbstractTask> & task) override;
    };*/
}

#endif
