#include "symbolic_search.h"


#include "../plugins/options.h"
#include "../plugins/plugin.h"

#include "../symbolic/sym_search.h"
#include "../symbolic/original_state_space.h"
#include "../symbolic/uniform_cost_search.h"
#include "../symbolic/bidirectional_search.h"
#include "../symbolic/sym_controller.h"

#include "../operator_cost.h"

using namespace std;
using namespace symbolic;
using plugins::Options;


namespace symbolic_search {

    SymbolicSearch::SymbolicSearch(const Options &opts) :
            SearchAlgorithm(opts), SymController(opts, task) { // TODO: Initialize task from somewhere else?
    }

    SymbolicBidirectionalUniformCostSearch::SymbolicBidirectionalUniformCostSearch(const Options &opts) :
            SymbolicSearch(opts) {
    }

    void SymbolicBidirectionalUniformCostSearch::initialize() {
        mgr = make_shared<OriginalStateSpace>(vars.get(), mgrParams, task);
        auto fw_search = make_unique<UniformCostSearch>(this, searchParams);
        auto bw_search = make_unique<UniformCostSearch>(this, searchParams);
        fw_search->init(mgr, true, mgr->getInitialState(), bw_search->getClosedShared());
        bw_search->init(mgr, false, mgr->getGoal(), fw_search->getClosedShared());

        search = make_unique<BidirectionalSearch>(this, searchParams, std::move(fw_search), std::move(bw_search));
    }


    SymbolicUniformCostSearch::SymbolicUniformCostSearch(const Options &opts, bool _fw) :
            SymbolicSearch(opts), fw(_fw) {
    }

    void SymbolicUniformCostSearch::initialize() {
        mgr = make_shared<OriginalStateSpace>(vars.get(), mgrParams, task);
        auto uni_search = make_unique<UniformCostSearch>(this, searchParams);
        if (fw) {
            uni_search->init(mgr, true);
        } else {
            uni_search->init(mgr, false);
        }

        search.reset(uni_search.release());
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

        if (getLowerBound() < getUpperBound()) {
            return IN_PROGRESS;
        } else if (found_solution()) {
            return SOLVED;
        } else {
            return FAILED;
        }
    }

    void SymbolicSearch::new_solution(const SymSolution &sol) {
        if (sol.getCost() < getUpperBound()) {
            vector<OperatorID> plan;
            sol.getPlan(plan);
            set_plan(plan);
        }

        SymController::new_solution(sol);
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

        virtual std::shared_ptr<SymbolicUniformCostSearch> create_component(
                const Options &options, const utils::Context &) const {
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

        virtual std::shared_ptr<SymbolicUniformCostSearch> create_component(
                const Options &options, const utils::Context &) const {
            return std::make_shared<SymbolicUniformCostSearch>(options, false);
        }

    };

    static plugins::FeaturePlugin<SymbolicBackwardUniformCostSearchFeature> _plugin_backward;
}
