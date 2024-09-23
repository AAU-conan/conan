#ifndef QUALIFIED_DOMINANCE_CONSTRAINTS_H
#define QUALIFIED_DOMINANCE_CONSTRAINTS_H

#include "constraint_generator.h"

namespace qdominance {
    class QualifiedDominancePruning;
}

namespace operator_counting {
    class QualifiedDominanceConstraints final : public ConstraintGenerator {
    public:
        void initialize_constraints(const std::shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) override;
        bool update_constraints(const State& state, lp::LPSolver& lp_solver) override;

    private:
        std::unique_ptr<qdominance::QualifiedDominancePruning> dominance_pruning;
    };
}

#endif //QUALIFIED_DOMINANCE_CONSTRAINTS_H
