#ifndef SYMBOLIC_SYM_ESTIMATE_H
#define SYMBOLIC_SYM_ESTIMATE_H

#include "../utils/timer.h"

#include <map>
#include <utility>
#include <iostream>
#include <fstream>

namespace symbolic {
class SymParamsSearch;
/*
 * NOTE: Though the aim of this class is to get an accurate estimation
 * of the times/nodes needed for an step, probably the overhead is not
 * worthy. Therefore, sym_estimate_linear.h is recommended.
 *
 * Class to estimate the cost of a given BDD operation depending on the size of the BDD.
 * It mantains a table that records previous experiences nodes => (cost in time, cost in nodes).
 * When it is asked about a new estimation, it
 *
 * We are assuming:
 * 1) estimations for larger BDDs are greater (we have to keep the consistency)
 * 2) If a new estimation that is not currently on our database is asked then:
 *    a) if the new value is larger than any in our data: linear interpolation with the largest one.
 *    b) if the new value is smaller than any in our data:
 *            just interpolate the estimations of the previous and next value.
 *
 * 3) When we relax a search we will only keep the current estimation
 * (the image is performed with other TRs, so all the other data is not relevant for us),
 */
class Estimation {
public:
    utils::Duration time;
    long nodes;
    Estimation(double t = 1, long n = 1) : time(t), nodes(n) {}

    friend std::ostream &operator<<(std::ostream &os, const Estimation &est);
};

class StepCostEstimation {
    //Parameters for the estimation
    double param_min_estimation_time;
    double param_penalty_time_estimation_sum, param_penalty_time_estimation_mult;
    long param_penalty_nodes_estimation_sum;
    double param_penalty_nodes_estimation_mult;

    long nextStepNodes; //Nodes of the step to be estimated
    Estimation estimation; //Current estimation of next step
    std::map<long, Estimation> data; //Data about time estimations (time, nodes)

    void update_data(long key, Estimation value);

public:
    StepCostEstimation(const SymParamsSearch &p);
    ~StepCostEstimation() {}

    void stepTaken(utils::Duration time, long nodes); //Called after any step, telling how much time was spent
    void nextStep(long nodes); //Called before any step, telling number of nodes to expand

    //Recompute the estimation if it has been exceeded
    void violated(utils::Duration time_ellapsed, utils::Duration time_limit, long node_limit);

    void recalculate(const StepCostEstimation &o, long nodes);

    inline utils::Duration time() const {
        return estimation.time;
    }

    inline long nodes() const {
        return estimation.nodes;
    }

    inline long nextNodes() const {
        return nextStepNodes;
    }

    inline void violated_nodes(long nodes) {
        violated(utils::Duration(0), utils::Duration(1), nodes);
    }

    friend std::ostream &operator<<(std::ostream &os, const StepCostEstimation &est);
};
}
#endif
