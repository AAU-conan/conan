#include "comparative_heuristic.h"

#include "../utils/component_errors.h"
#include "../plugins/plugin.h"
#include <ranges>
#include <format>
#include <boost/algorithm/string/join.hpp>
#include <print>

class Evaluator;

ComparativeHeuristic::ComparativeHeuristic(const std::vector<std::shared_ptr<Evaluator>> &comparison_heuristics,
    const std::shared_ptr<AbstractTask> &task, bool cache_estimates, const std::string &description,
    utils::Verbosity verbosity): Heuristic(task, cache_estimates, description, verbosity), comparison_heuristics(comparison_heuristics) {
    utils::verify_list_not_empty(comparison_heuristics, "heuristics");
}

EvaluationResult ComparativeHeuristic::compute_result(EvaluationContext& eval_context) {
    const auto results = comparison_heuristics | std::views::transform([&eval_context](const auto& heuristic) {
        return heuristic->compute_result(eval_context);
    }) | std::ranges::to<std::vector<EvaluationResult>>();
    std::println("{}", boost::algorithm::join(std::views::zip(comparison_heuristics, results) | std::views::transform([](const std::pair<std::shared_ptr<Evaluator>, EvaluationResult>& pair) {
        return std::format("{}: {}", pair.first->get_description(), pair.second.get_evaluator_value());
    }) | std::ranges::to<std::vector<std::string>>(), ", "));
    return results.front();
}

int ComparativeHeuristic::compute_heuristic(const State& ancestor_state) {
    utils::unused_variable(ancestor_state);
    throw std::runtime_error("Should not be reachable");
}


class ComparativeHeuristicFeature
    : public plugins::TypedFeature<Evaluator, ComparativeHeuristic> {
public:
    ComparativeHeuristicFeature() : TypedFeature("comparative") {
        document_title("Compare Multiple Heuristics");
        document_synopsis("Compare multiple heuristics by running search with the first heuristic, but evaluating all heuristics for each state.");

        add_list_option<std::shared_ptr<Evaluator>>(
            "heuristics",
            "Heuristics to compare, the first in the list will be used for search");

        add_heuristic_options_to_feature(*this, "comparative");
    }

    virtual std::shared_ptr<ComparativeHeuristic> create_component(
        const plugins::Options &opts) const override {
        return plugins::make_shared_from_arg_tuples<ComparativeHeuristic>(
            opts.get_list<std::shared_ptr<Evaluator>>( "heuristics"),
            get_heuristic_arguments_from_options(opts)
        );
    }
};

static plugins::FeaturePlugin<ComparativeHeuristicFeature> _plugin;

