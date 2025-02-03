#ifndef QUALIFIED_DOMINANCE_CONSTRAINTS_H
#define QUALIFIED_DOMINANCE_CONSTRAINTS_H

#include "constraint_generator.h"
#include "../lp/lp_solver.h"
#include "../qualified_dominance/qualified_ld_simulation.h"

namespace fts {
    struct TransformedFTSTask;
}

namespace dom {
    class QualifiedDominanceAnalysis;
    class IncrementalLDSimulation;
    class QualifiedDominancePruning;
}

namespace operator_counting {
    class QualifiedDominanceConstraints final : public ConstraintGenerator {
    public:
        void add_automaton_to_lp(const mata::nfa::Nfa& automaton, lp::LinearProgram& lp, mata::nfa::State state,
                                 int factor);
        void initialize_constraints(const std::shared_ptr<AbstractTask> &task, lp::LinearProgram &lp) override;
        bool update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) override;

        std::shared_ptr<DominanceAnalysis> dominance_analysis;

        bool only_pruning;
        bool minimize_nfa;
        bool approximate_determinization;


        explicit QualifiedDominanceConstraints(std::shared_ptr<DominanceAnalysis> dominance_analysis, const bool only_pruning = false, const bool minimize_nfa = true, const bool approximate_determinization = false) : dominance_analysis(std::move(dominance_analysis)), only_pruning(only_pruning), minimize_nfa(minimize_nfa), approximate_determinization(approximate_determinization)
        {}

    private:
        std::unique_ptr<TransformedFTSTask> transformed_task;
        std::unique_ptr<FactoredDominanceRelation> factored_domrel = nullptr;
        struct GValuedState {
            std::vector<int> state;
            int g_value;
        };
        std::vector<GValuedState> previous_states;

        // For factor i, the map from state s, t to the state in the automaton that represents that t simulates s
        std::vector<std::vector<std::vector<mata::nfa::State>>> state_pair_to_nfa_state;

        // For factor i, flow from state j, and transition k in the NFA, the variable that represents the transition
        std::vector<std::vector<std::vector<int>>> transition_variables;
        // For factor i, and state j in the NFA, the variable that represents that there should be flow from state q
        std::vector<std::vector<int>> init_variables;
    };
}

#endif //QUALIFIED_DOMINANCE_CONSTRAINTS_H
