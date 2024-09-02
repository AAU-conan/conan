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
        std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering;
        std::shared_ptr<SymVariables> vars; //The symbolic variables are declared here

        SymParamsMgr mgrParams; //Parameters for SymStateSpaceManager configuration.
        SymParamsSearch searchParams; //Parameters to search the original state space

        int lower_bound;
        SymSolution solution;
    public:
        SymController(const plugins::Options &opts, std::shared_ptr<AbstractTask> task);

        virtual ~SymController() = default;

        virtual void new_solution(const SymSolution &sol);

        void setLowerBound(int lower);

        int getUpperBound() const {
            if (solution.solved()) return solution.getCost();
            else return std::numeric_limits<int>::max();
        }

        std::string getUpperBoundString() const;

        int getLowerBound() const {
            return lower_bound;
        }

        bool solved() const {
            return getLowerBound() >= getUpperBound();
        }

        const SymSolution *get_solution() const {
            return &solution;
        }

        inline SymVariables *getVars() {
            return vars.get();
        }

        inline const SymParamsMgr &getMgrParams() const {
            return mgrParams;
        }

        inline const SymParamsSearch &getSearchParams() const {
            return searchParams;
        }

        static void add_options_to_feature(plugins::Feature &parser, utils::Duration maxStepTime, long maxStepNodes);
    };
}
#endif
