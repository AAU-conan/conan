#include "comparative_heuristic.h"
#include "../plugins/plugin.h"
#include <print>
#include <ranges>
#include <format>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/detail/classification.hpp>

class Evaluator;

EvaluationResult ComparativeHeuristic::compute_result(EvaluationContext& eval_context) {
    std::vector<EvaluationResult> results;
    for (const auto& heuristic : comparison_heuristics) {
        results.push_back(heuristic->compute_result(eval_context));
    }
    for (int i = 0; i < comparison_heuristics.size(); ++i) {
        std::cout << comparison_heuristics.at(i)->get_description() << ": " << results.at(i).get_evaluator_value() << ", ";
    }
    std::cout << std::endl;
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

