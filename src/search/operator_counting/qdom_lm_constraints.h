#ifndef QDOM_LANDMARK_CONSTRAINTS_H
#define QDOM_LANDMARK_CONSTRAINTS_H

#include "constraint_generator.h"
#include "../lp/lp_solver.h"
#include "../qualified_dominance/qualified_ld_simulation.h"

#include <mata/nfa/nfa.hh>

namespace qdominance {
    class QualifiedDominanceAnalysis;
    class QualifiedLDSimulation;
    class QualifiedDominancePruning;
}

namespace operator_counting {
    class QualifiedDominanceLandmarkConstraints final : public ConstraintGenerator {
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

        // For factor i, state q, the operators from the stratum of q to the next stratum
        std::vector<std::vector<std::vector<int>>> landmark_ops;

        // For factor i, state q, the states that q can reach
        std::vector<std::vector<std::set<mata::nfa::State>>> reachable;

        // For factor i, state q, the stratum q belongs to
        std::vector<std::vector<int>> state_to_stratum;

        // For factor i, the stratum j, the states that belong to stratum j
        std::vector<std::vector<std::vector<mata::nfa::State>>> stratification;
    };
}

#endif //QDOM_LANDMARK_CONSTRAINTS_H
