#include "sym_controller.h"

#include "sym_state_space_manager.h"
#include "bidirectional_search.h"

#include "../plugins/options.h"
#include "../plugins/plugin.h"

#include <memory>

using namespace std;
using plugins::Options;
using plugins::Feature;

namespace variable_ordering {
    class VariableOrderingStrategy;
}

namespace symbolic {
    SymController::SymController(const Options &opts, shared_ptr <AbstractTask> _task)
            : bdd_manager(make_shared<BDDManager>(opts)),
              variable_ordering(opts.get < shared_ptr < variable_ordering::VariableOrderingStrategy>> ("variable_ordering")),
              mgrParams(opts), searchParams(opts), lower_bound(0), solution() {
        mgrParams.print_options();
        searchParams.print_options();
        vars = std::make_shared<SymVariables>(bdd_manager, *variable_ordering, _task);
    }


    void SymController::add_options_to_feature(Feature &feature, utils::Duration maxStepTime, long maxStepNodes) {
        BDDManager::add_options_to_feature(feature);
        SymParamsMgr::add_options_to_feature(feature);
        SymParamsSearch::add_options_to_feature(feature, maxStepTime, maxStepNodes);
        feature.add_option<shared_ptr<variable_ordering::VariableOrderingStrategy>>(
                "variable_ordering",
                "Strategy to decide the variable ordering",
                "gamer()");
    }

    void SymController::new_solution(const SymSolution &sol) {
        if (!solution.solved() ||
            sol.getCost() < solution.getCost()) {
            solution = sol;
            utils::g_log << "BOUND: " << lower_bound << " < " << getUpperBoundString() << std::endl;

        }
    }

    void SymController::setLowerBound(int lower) {
        //Never set a lower bound greater than the current upper bound
        if (solution.solved()) {
            lower = min(lower, solution.getCost());
        }

        if (lower > lower_bound) {
            lower_bound = lower;

            utils::g_log << "BOUND: " << lower_bound << " < " << getUpperBoundString() << std::endl;

        }

    }

    std::string SymController::getUpperBoundString() const {
        if (solution.solved()) return std::to_string(solution.getCost());
        else return "infinity";
    }
}
