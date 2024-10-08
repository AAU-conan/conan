#ifndef CONTRASTIVE_LANDMARKS_H
#define CONTRASTIVE_LANDMARKS_H

#include "constraint_generator.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../lp/lp_solver.h"

namespace operator_counting {
    class ContrastiveLandmarks final : public ConstraintGenerator {
    public:
        void initialize_constraints(const std::shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) override;
        bool update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) override;

    private:
        std::unique_ptr<fts::FTSTask> fts_task = nullptr;

        struct GValuedState {
            std::vector<int> state;
            int g_value;
        };
        std::vector<GValuedState> previous_states;
    };
}

#endif //CONTRASTIVE_LANDMARKS_H
