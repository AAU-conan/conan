#include "qualified_dominance_constraints.h"

#include "../qualified_dominance/qualified_dominance_pruning_local.h"

namespace operator_counting {
    void QualifiedDominanceConstraints::initialize_constraints(const std::shared_ptr<AbstractTask>& task, lp::LinearProgram& lp) {
    }

    bool QualifiedDominanceConstraints::update_constraints(const State& state, lp::LPSolver& lp_solver) {
        return false;
    }
}
