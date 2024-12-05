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
            : bdd_manager(make_shared<BDDManager>(opts)), mgrParams(opts), searchParams(opts) {

        auto variable_ordering = opts.get < shared_ptr < variable_ordering::VariableOrderingStrategy>> ("variable_ordering");
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
}
