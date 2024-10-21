//
// Created by rasmus on 21-10-24.
//

#ifndef FACTORED_CONSTRAINTS_H
#define FACTORED_CONSTRAINTS_H

#include "constraint_generator.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../lp/lp_solver.h"
namespace operator_counting {
    class FactoredConstraints final : public ConstraintGenerator {
    public:
        void initialize_constraints(const std::shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) override;
        bool update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) override;
        void add_factor_lts_to_lp(const fts::LabelledTransitionSystem& lts, lp::LinearProgram& lp, int factor);

    private:
        std::unique_ptr<fts::FTSTask> fts_task = nullptr;

        // For factor i, transition j, the variable that represents the transition
        std::vector<std::vector<int>> lts_transition_variables;
        // For factor i, and state j in the lts, the variable that represents that the explicit state is j
        std::vector<std::vector<int>> lts_init_variables;
    };
}



#endif //FACTORED_CONSTRAINTS_H
