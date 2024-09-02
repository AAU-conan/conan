#include "sym_search.h"

#include "../plugins/options.h"
#include "../plugins/plugin.h"

using plugins::Options;
using plugins::Feature;

using namespace std;
using utils::g_timer;

namespace symbolic {
    SymSearch::SymSearch(SymController *eng, const SymParamsSearch &params) :
            mgr(nullptr), p(params), engine(eng) {}


    SymParamsSearch::SymParamsSearch(const Options &opts) :
            max_disj_nodes(opts.get<int>("max_disj_nodes")),
            min_estimation_time(opts.get<double>("min_estimation_time")),
            penalty_time_estimation_sum(opts.get<double>("penalty_time_estimation_sum")),
            penalty_time_estimation_mult(opts.get<double>("penalty_time_estimation_mult")),
            penalty_nodes_estimation_sum(opts.get<int>("penalty_nodes_estimation_sum")),
            penalty_nodes_estimation_mult(opts.get<double>("penalty_nodes_estimation_mult")),
            maxStepTime(opts.get<double>("max_step_time")),
            maxStepNodes(opts.get<int>("max_step_nodes")),
            minAllottedTime(opts.get<double>("min_allotted_time")),
            maxAllottedTime(opts.get<double>("max_allotted_time")),
            minAllottedNodes(opts.get<int>("min_allotted_nodes")),
            maxAllottedNodes(opts.get<int>("max_allotted_nodes")),
            ratioAllottedTime(opts.get<double>("ratio_allotted_time")),
            ratioAllottedNodes(opts.get<double>("ratio_allotted_nodes")),
            non_stop(opts.get<bool>("non_stop")),
            log(utils::get_log_for_verbosity(std::get<0>(utils::get_log_arguments_from_options(opts)))) {
    }

    void SymParamsSearch::print_options() const {
        if (log.is_at_least_verbose()) {
            log << "Disj(nodes=" << max_disj_nodes << ")" << endl;
            log << "Estimation: min_time(" << min_estimation_time << ")" <<
                " time_penalty +(" << penalty_time_estimation_sum << ")" <<
                "*(" << penalty_time_estimation_mult << ")" <<
                " nodes_penalty +(" << penalty_nodes_estimation_sum << ")" <<
                "*(" << penalty_nodes_estimation_mult << ")" << endl;
            log << "   Min allotted time: " << minAllottedTime << " nodes: " << minAllottedNodes << endl;
            log << "   Max allotted time: " << maxAllottedTime << " nodes: " << maxAllottedNodes << endl;
            log << "   Mult allotted time: " << ratioAllottedTime << " nodes: " << ratioAllottedNodes << endl;
        }
    }

    void SymParamsSearch::add_options_to_feature(Feature &feature, utils::Duration maxStepTime, long maxStepNodes) {
        feature.add_option<int>("max_disj_nodes",
                                "maximum size to enforce disjunction before image", "infinity");

        feature.add_option<double>("min_estimation_time",
                                   "minimum time in seconds to perform linear interpolation for estimation", "1");
        feature.add_option<double>("penalty_time_estimation_sum", "time in seconds added when violated allotted time",
                                   "1");
        feature.add_option<double>("penalty_time_estimation_mult", "multiplication factor when violated allotted time",
                                   "2");
        feature.add_option<int>("penalty_nodes_estimation_sum", "nodes added when violated allotted nodes", "1000");
        feature.add_option<double>("penalty_nodes_estimation_mult",
                                   "multiplication factor when violated allotted nodes", "2");

        feature.add_option<double>("max_step_time", "allowed time to perform a step in the search",
                                   std::to_string(maxStepTime));
        feature.add_option<int>("max_step_nodes", "allowed nodes to perform a step in the search",
                                std::to_string(maxStepNodes));

        //The default value is a 50% percent more than maxStepTime,
        feature.add_option<double>("min_allotted_time", "minimum allotted time for an step", "60");
        feature.add_option<int>("min_allotted_nodes", "minimum allotted nodes for an step", to_string((int) 10e6));

        feature.add_option<double>("max_allotted_time", "maximum allotted time for an step", "60");
        feature.add_option<int>("max_allotted_nodes", "maximum allotted nodes for an step", to_string((int) 15e6));

        feature.add_option<double>("ratio_allotted_time", "multiplier to decide allotted time for a step", "2.0");
        feature.add_option<double>("ratio_allotted_nodes", "multiplier to decide allotted nodes for a step", "2.0");

        feature.add_option<bool>("non_stop",
                                 "Removes initial state from closed to avoid backward search to stop.",
                                 "false");
    }

    int SymParamsSearch::getMaxStepNodes() const {
        return maxStepNodes;
    }

}
