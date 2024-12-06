#ifndef SYMBOLIC_ORIGINAL_STATE_SPACE_H
#define SYMBOLIC_ORIGINAL_STATE_SPACE_H

#include "sym_state_space_manager.h"

namespace symbolic {
    // TODO: Not clear that we actually need inheritance here, just for the tag and storing the indTrs
    class OriginalStateSpace : public SymStateSpaceManager {

//    void init_mutex (const std::vector<MutexGroup> &mutex_groups);
//    void init_mutex(const std::vector<MutexGroup> &mutex_groups,
//		    bool genMutexBDD, bool genMutexBDDByFluent, bool fw);

        std::shared_ptr<AbstractTask> task;

        // Individual TRs: Useful for shrink and plan construction
        std::map<int, std::vector<std::shared_ptr<TransitionRelation>>> indTRs;

    public:
        OriginalStateSpace(SymVariables *v, const SymParamsMgr &params, const std::shared_ptr<AbstractTask> &_task);

        virtual std::string tag() const override {
            return "original";
        }

        const std::map<int, std::vector<std::shared_ptr<TransitionRelation>>> &getIndividualTRs() const override {
            return indTRs;
        }

        virtual ~OriginalStateSpace() = default;

        virtual TaskProxy getTask() const override {
            return TaskProxy(*task);
        }
    };
}
#endif
