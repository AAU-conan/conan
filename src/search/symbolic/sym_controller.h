#ifndef SYMBOLIC_SYM_CONTROLLER_H
#define SYMBOLIC_SYM_CONTROLLER_H

#include "sym_state_space_manager.h"
#include "sym_solution.h"
#include "sym_search.h"

#include <vector>
#include <memory>
#include <limits>

namespace plugins {
    class Feature;

    class Options;
}

namespace symbolic {
    class SymSolution;

    class SymVariables;

    class SymController {
    protected:
        std::shared_ptr<BDDManager> bdd_manager;
        std::shared_ptr<SymVariables> vars; //The symbolic variables are declared here

        SymParamsMgr mgrParams; //Parameters for SymStateSpaceManager configuration.
        SymParamsSearch searchParams; //Parameters to search the original state space


    public:
        SymController(const plugins::Options &opts, std::shared_ptr<AbstractTask> task);

        virtual ~SymController() = default;

        inline SymVariables *getVars() {
            return vars.get();
        }

        static void add_options_to_feature(plugins::Feature &parser, utils::Duration maxStepTime, long maxStepNodes);
    };
}
#endif
