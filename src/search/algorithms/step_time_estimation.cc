#include "step_time_estimation.h"

#include <algorithm>
#include <iostream>

#include "../symbolic/sym_search.h"

using namespace std;

namespace symbolic {
StepCostEstimation::StepCostEstimation(const SymParamsSearch &p) :
    param_min_estimation_time(p.min_estimation_time),
    param_penalty_time_estimation_sum(p.penalty_time_estimation_sum),
    param_penalty_time_estimation_mult(p.penalty_time_estimation_mult),
    param_penalty_nodes_estimation_sum(p.penalty_nodes_estimation_sum),
    param_penalty_nodes_estimation_mult(p.penalty_nodes_estimation_mult),
    nextStepNodes(1) {
    //Initialize the first data points (useful for interpolation)
    data[0] = Estimation(1, 1);
    data[1] = Estimation(1, 1);
}

void StepCostEstimation::update_data(long key, Estimation est) {
    if (!data.count(key)) {
        //Ensure consistency with lower estimations, we do not store the data in case is smaller
        auto it = data.lower_bound(key);
        it--; //Guaranteed to exist because we initialize data[0] and data[1]
        est.time = max(est.time, it->second.time);
        est.nodes = max(est.nodes, it->second.nodes);
    } else if (est.time < data[key].time && est.nodes < data[key].nodes) {
        return; //Also skip in case that the new value is smaller than the previous
    } else {
        est.time = max(est.time, data[key].time);
        est.nodes = max(est.nodes, data[key].nodes);
    }

    data[key] = est;

    //Ensure consistency with greater estimations
    for (auto it = data.upper_bound(nextStepNodes); it != end(data); ++it) {
        if (it->second.time < est.time) {
            it->second.time = est.time;
        }
        if (it->second.nodes < est.nodes) {
            it->second.nodes = est.nodes;
        }
    }
}

void StepCostEstimation::stepTaken(utils::Duration time, long nodes) {
#ifdef DEBUG_ESTIMATES
    cout << "== STEP TAKEN: " << time << ", " << nodes << endl;
#endif
    update_data(nextStepNodes, Estimation(time + 10, nodes)); //consider 10ms more to avoid values close to 0
}

//Sets the nodes of next iteration and recalculate estimations
void StepCostEstimation::nextStep(long nodes) {
#ifdef DEBUG_ESTIMATES
    cout << "== NEXT STEP: " << nodes << " " << *this << " to ";
#endif
    nextStepNodes = nodes;

    if (data.count(nodes)) {
        return; //We already have an estimation in our data :D
    }

    double estimatedTime;
    long estimatedNodes;
    //Get next data point
    auto nextIt = data.upper_bound(nextStepNodes);
    if (nextIt == end(data)) {
        //This is greater than any est we have, just get the greatest
        --nextIt;
        long prevNodes = nextIt->first;
        Estimation prevEst = nextIt->second;

        if (prevNodes <= param_min_estimation_time) {
            estimatedTime = prevEst.time;
            estimatedNodes = prevEst.nodes;
        } else {
            long incrementNodes = std::max<long>(0, nextStepNodes - prevEst.nodes);
            estimatedNodes = nextStepNodes + incrementNodes;

            double proportionNodes = ((double)estimatedNodes) / ((double)nextStepNodes);
            estimatedTime = prevEst.time * proportionNodes;
        }
    } else {
        long nextNodes = nextIt->first;
        Estimation nextEst = nextIt->second;
        --nextIt; //Guaranteed to exist (because we have initialized data[0] and data[1])
        long prevNodes = nextIt->first;
        Estimation prevEst = nextIt->second;

        //Interpolate
        double percentage = ((double)(nextStepNodes - prevNodes)) / ((double)(nextNodes - prevNodes));
        estimatedTime = prevEst.time + percentage * (nextEst.time - prevEst.time);
        estimatedNodes = static_cast<long>(prevEst.nodes + percentage * (nextEst.nodes - prevEst.nodes));
    }

    estimation = Estimation(estimatedTime, estimatedNodes);
#ifdef DEBUG_ESTIMATES
    cout << *this << endl;
    if (this->nodes() <= 0) {
        cout << "ERROR: estimated nodes is lower than 0 after nextStep" << endl;
        utils::exit_with(utils::ExitCode::CRITICAL_ERROR);
    }
#endif
}

void StepCostEstimation::violated(utils::Duration time_ellapsed, utils::Duration time_limit, long node_limit) {
#ifdef DEBUG_ESTIMATES
    cout << "== VIOLATED " << *this << ": " << time_ellapsed << " " << time_limit << " " << node_limit << ", ";
#endif
    estimation.time = utils::Duration(param_penalty_time_estimation_sum +
                      max<double>(estimation.time, time_ellapsed) * param_penalty_time_estimation_mult);

    estimation.nodes = nodes();
    if (time_ellapsed < time_limit) {
        estimation.nodes = static_cast<long>(param_penalty_nodes_estimation_sum +
                                             std::max<long>(estimation.nodes, node_limit) *
                                             param_penalty_nodes_estimation_mult);
    }

    update_data(nextStepNodes, estimation);
#ifdef DEBUG_ESTIMATES
    cout << *this << endl;
#endif
}

void StepCostEstimation::recalculate(const StepCostEstimation &o, long nodes) {
    nextStepNodes = nodes;
    double proportion = (double)nextStepNodes / (double)(o.nextStepNodes);
    estimation = Estimation(o.time() * proportion, static_cast<long>(o.nodes() * proportion));
    update_data(nodes, estimation);
}

ostream &operator<<(ostream &os, const StepCostEstimation &est) {
    return os << est.nextStepNodes << " nodes => (" <<
           est.time() << " ms, " << est.nodes() << " nodes)";
}

ostream &operator<<(ostream &os, const Estimation &est) {
    return os << est.time << ", " << est.nodes;
}
}
