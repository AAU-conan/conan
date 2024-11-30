#ifndef COMPARATIVE_HEURISTIC_H
#define COMPARATIVE_HEURISTIC_H

#include "../heuristic.h"

class ComparativeHeuristic final : public Heuristic {
    std::vector<std::shared_ptr<Evaluator>> comparison_heuristics;
public:
    ComparativeHeuristic(const std::vector<std::shared_ptr<Evaluator>>& comparison_heuristics, const std::shared_ptr<AbstractTask>& task, bool cache_estimates,
        const std::string& description, utils::Verbosity verbosity)
        : Heuristic(task, cache_estimates, description, verbosity), comparison_heuristics(comparison_heuristics) {
    }

    EvaluationResult compute_result(EvaluationContext& eval_context) override;

protected:
    int compute_heuristic(const State& ancestor_state) override;
};



#endif //COMPARATIVE_HEURISTIC_H
