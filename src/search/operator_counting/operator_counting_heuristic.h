#ifndef OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H
#define OPERATOR_COUNTING_OPERATOR_COUNTING_HEURISTIC_H

#include "../heuristic.h"

#include "../lp/lp_solver.h"

#include <memory>
#include <vector>

namespace plugins {
class Options;
}

namespace operator_counting {
class ConstraintGenerator;

class OperatorCountingHeuristic : public Heuristic {
protected:
    std::vector<std::shared_ptr<ConstraintGenerator>> constraint_generators;
    lp::LPSolver lp_solver;
    named_vector::NamedVector<lp::LPVariable> lp_variables;
    virtual int compute_heuristic(const State &ancestor_state) override;
public:
    OperatorCountingHeuristic(
        const std::vector<std::shared_ptr<ConstraintGenerator>>
        &constraint_generators,
        bool use_integer_operator_counts, lp::LPSolverType lpsolver,
        const std::shared_ptr<AbstractTask> &transform,
        bool cache_estimates, const std::string &description,
        utils::Verbosity verbosity);
};


class GDependentOperatorCountingHeuristic : public OperatorCountingHeuristic {
protected:
    int current_g_value = -1;

public:
    GDependentOperatorCountingHeuristic(const std::vector<std::shared_ptr<ConstraintGenerator>>& constraint_generators,
        bool use_integer_operator_counts, lp::LPSolverType lpsolver, const std::shared_ptr<AbstractTask>& transform,
        bool cache_estimates, const std::string& description, utils::Verbosity verbosity)
        : OperatorCountingHeuristic(
            constraint_generators, use_integer_operator_counts, lpsolver, transform, cache_estimates, description,
            verbosity) {
    }

protected:
    EvaluationResult compute_result(EvaluationContext &eval_context) override;
    int compute_heuristic(const State &ancestor_state) override;
};
}

#endif
