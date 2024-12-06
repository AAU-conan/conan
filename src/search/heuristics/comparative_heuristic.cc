#include "comparative_heuristic.h"
#include "../plugins/plugin.h"
#include <ranges>
#include <format>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/detail/classification.hpp>
#include <print>

class Evaluator;

EvaluationResult ComparativeHeuristic::compute_result(EvaluationContext& eval_context) {
    const auto results = comparison_heuristics | std::views::transform([&eval_context](const auto& heuristic) {
        return heuristic->compute_result(eval_context);
    }) | std::ranges::to<std::vector<EvaluationResult>>();
    std::println("{}", boost::algorithm::join(std::views::zip(comparison_heuristics, results) | std::views::transform([](const std::pair<std::shared_ptr<Evaluator>, EvaluationResult>& pair) {
        return std::format("{}: {}", pair.first->get_description(), pair.second.get_evaluator_value());
    }) | std::ranges::to<std::vector<std::string>>(), ", "));
    if (results.at(1).get_evaluator_value() < results.at(2).get_evaluator_value()) {
        std::println("NOT STRICTLY BETTER!");
    }
    return results.front();
}

int ComparativeHeuristic::compute_heuristic(const State& ancestor_state) {
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
        const plugins::Options &opts,
        const utils::Context &context) const override {
        plugins::verify_list_non_empty<std::shared_ptr<Evaluator>>(
            context, opts, "heuristics");
        return plugins::make_shared_from_arg_tuples<ComparativeHeuristic>(
            opts.get_list<std::shared_ptr<Evaluator>>( "heuristics"),
            get_heuristic_arguments_from_options(opts)
        );
    }
};

static plugins::FeaturePlugin<ComparativeHeuristicFeature> _plugin;

