#ifndef SYMBOLIC_HEURISTICS_SYMBOLIC_PERIMETER_HEURISTIC_H
#define SYMBOLIC_HEURISTICS_SYMBOLIC_PERIMETER_HEURISTIC_H

#include "../heuristic.h"
#include "../symbolic/sym_controller.h"

namespace symbolic {
    class SymbolicPerimeterHeuristic : public Heuristic {
        const int generation_time;
        const double generation_memory;

        std::shared_ptr<SymVariables> vars;
        std::unique_ptr<ADD> perimeter_heuristic;
        // std::vector<BDD> notMutexBDDs;
    protected:
        int compute_heuristic(const State &state) override;

    public:
        SymbolicPerimeterHeuristic(int generation_time, double generation_memory,
                                   std::shared_ptr<BDDManager> bdd_manager,
                                   const std::shared_ptr<variable_ordering::VariableOrderingStrategy> &variable_ordering,
                                   const SymParamsMgr &mgrParams,
                                   const SymParamsSearch &searchParams,
                                   const std::shared_ptr<AbstractTask> &transform,
                                   bool cache_estimates, const std::string &description,
                                   utils::Verbosity verbosity);

        virtual ~SymbolicPerimeterHeuristic() = default;
    };
}

#endif
