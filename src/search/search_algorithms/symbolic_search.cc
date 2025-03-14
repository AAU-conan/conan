#include "symbolic_search.h"


#include "../plugins/options.h"
#include "../plugins/plugin.h"

#include "../symbolic/sym_search.h"
#include "../symbolic/original_state_space.h"
#include "../symbolic/uniform_cost_search.h"
#include "../symbolic/bidirectional_search.h"
#include "../symbolic/opposite_frontier.h"
#include "../symbolic/sym_controller.h"

using namespace std;
using namespace symbolic;
using plugins::Options;


namespace symbolic_search {
    SymbolicSearch::SymbolicSearch(const Options &opts) : SearchAlgorithm(opts), bdd_manager (make_shared<BDDManager>(opts)) {
        // TODO: Initialize task from somewhere else?
        auto variable_ordering = opts.get<shared_ptr<variable_ordering::VariableOrderingStrategy> >("variable_ordering");
        SymParamsMgr mgr_params(opts);

        vars = std::make_shared<SymVariables>(bdd_manager, *variable_ordering, task);
        mgr = make_shared<OriginalStateSpace>(vars.get(), mgr_params, task);
        solution = make_shared<SymSolutionLowerBound>();
        solution->add_notifier(this);
    }

    SymbolicBidirectionalUniformCostSearch::SymbolicBidirectionalUniformCostSearch(const Options &opts) : SymbolicSearch(opts) {
        SymParamsSearch search_params(opts);

        auto fw_closed = make_shared<ClosedList>(mgr);
        auto bw_closed = make_shared<ClosedList>(mgr);

        auto fw_search = make_unique<UniformCostSearch>(search_params, mgr, true, mgr->getInitialState(), fw_closed, make_shared<OppositeFrontierClosed>(bw_closed), solution);
        auto bw_search = make_unique<UniformCostSearch>(search_params, mgr, false, mgr->getGoal(), bw_closed, make_shared<OppositeFrontierClosed>(fw_closed), solution);

        search = make_unique<BidirectionalSearch>(search_params, std::move(fw_search), std::move(bw_search));
    }


    SymbolicUniformCostSearch::SymbolicUniformCostSearch(const Options &opts,
                                                         bool _fw) : SymbolicSearch(opts), fw(_fw) {
        search = make_unique<UniformCostSearch>(SymParamsSearch(opts), mgr, fw, solution);
    }

    /*
        void SymbolicUniformCostSearch::handle_unsolvable_problem() {
            BDD seen = search->get_seen_states(true);

            cout << "States reached: " << vars->numStates(seen) << endl;
        }
    */


    void SymbolicSearch::print_statistics() const {
        // TODO: print statistics
    }

    SearchStatus SymbolicSearch::step() {
        search->step();

        if (solution->getLowerBound() < solution->getUpperBound()) {
            return IN_PROGRESS;
        } else if (found_solution()) {
            return SOLVED;
        } else {
            return FAILED;
        }
    }

    void SymbolicSearch::notify_solution(const SymSolution &sol) {
        vector<OperatorID> plan;
        sol.getPlan(plan);
        set_plan(plan);
    }

    class SymbolicBidirectionalUniformCostSearchFeature
            : public plugins::TypedFeature<SearchAlgorithm, SymbolicBidirectionalUniformCostSearch> {
    public:
        SymbolicBidirectionalUniformCostSearchFeature() : TypedFeature("sbd") {
            document_title("Symbolic bidirectional uniform-cost search");
            document_synopsis("");

            add_search_algorithm_options_to_feature(*this, "sbd");
            SymController::add_options_to_feature(*this, utils::Duration(30), 10e7);
        }
    };

    static plugins::FeaturePlugin<SymbolicBidirectionalUniformCostSearchFeature> _plugin_bidirectional;

    class SymbolicForwardUniformCostSearchFeature
            : public plugins::TypedFeature<SearchAlgorithm, SymbolicUniformCostSearch> {
    public:
        SymbolicForwardUniformCostSearchFeature() : TypedFeature("sfw") {
            document_title("Symbolic unidirectional uniform-cost search");
            document_synopsis("");

            add_search_algorithm_options_to_feature(*this, "sfw");
            SymController::add_options_to_feature(*this, utils::Duration(30), 10e7);
        }

        std::shared_ptr<SymbolicUniformCostSearch> create_component(const Options &options) const override {
            return std::make_shared<SymbolicUniformCostSearch>(options, true);
        }
    };

    static plugins::FeaturePlugin<SymbolicForwardUniformCostSearchFeature> _plugin_forward;


    class SymbolicBackwardUniformCostSearchFeature
            : public plugins::TypedFeature<SearchAlgorithm, SymbolicUniformCostSearch> {
    public:
        SymbolicBackwardUniformCostSearchFeature() : TypedFeature("sbw") {
            document_title("Symbolic unidirectional uniform-cost search");
            document_synopsis("");

            add_search_algorithm_options_to_feature(*this, "sbw");
            SymController::add_options_to_feature(*this, utils::Duration(30), 10e7);
        }

        std::shared_ptr<SymbolicUniformCostSearch> create_component(const Options &options) const override {
            return std::make_shared<SymbolicUniformCostSearch>(options, false);
        }
    };

    static plugins::FeaturePlugin<SymbolicBackwardUniformCostSearchFeature> _plugin_backward;
}
