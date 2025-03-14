#ifndef OPERATOR_COUNTING_G_DEPENDENT_OPERATOR_COUNTING_HEURISTIC_H
#define OPERATOR_COUNTING_G_DEPENDENT_OPERATOR_COUNTING_HEURISTIC_H

#include "operator_counting_heuristic.h"

namespace operator_counting {
    class GDependentOperatorCountingHeuristic : public operator_counting::OperatorCountingHeuristic {
    protected:
        int current_g_value = -1;

    public:
        GDependentOperatorCountingHeuristic(const std::vector<std::shared_ptr<ConstraintGenerator>>& constraint_generators,
            bool use_integer_operator_counts, lp::LPSolverType lpsolver, const std::shared_ptr<AbstractTask>& transform,
            bool cache_estimates, const std::string& description, utils::Verbosity verbosity);

    protected:
        EvaluationResult compute_result(EvaluationContext &eval_context) override;
        int compute_heuristic(const State &ancestor_state) override;
    };
}

#endif
