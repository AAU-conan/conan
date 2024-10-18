#ifndef QUALIFIED_DOMINANCE_CONSTRAINTS_H
#define QUALIFIED_DOMINANCE_CONSTRAINTS_H

#include "constraint_generator.h"
#include "../dominance/dominance_pruning_local.h"
#include "../lp/lp_solver.h"
#include "../qualified_dominance/qualified_ld_simulation.h"

namespace qdominance {
    class QualifiedDominanceAnalysis;
    class QualifiedLDSimulation;
    class QualifiedDominancePruning;
}

namespace operator_counting {
    class QualifiedDominanceConstraints final : public ConstraintGenerator {
    public:
        void initialize_constraints(const std::shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) override;
        bool update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) override;

    private:
        std::unique_ptr<qdominance::QualifiedFactoredDominanceRelation> factored_qdomrel = nullptr;
        struct GValuedState {
            std::vector<int> state;
            int g_value;
        };
        std::vector<GValuedState> previous_states;

        // For factor i, flow from state j, and transition k in the NFA, the variable that represents the transition
        std::vector<std::vector<std::vector<int>>> transition_variables;
        // For factor i, flow from state j, the variable that represent the goal variable for the unique final state
        // std::vector<std::vector<int>> goal_variables;
        // For factor i, and state j in the NFA, the variable that represents that there should be flow from state q
        std::vector<std::vector<int>> init_variables;
    };
}

#endif //QUALIFIED_DOMINANCE_CONSTRAINTS_H
