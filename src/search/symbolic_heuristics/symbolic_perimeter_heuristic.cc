#include "symbolic_perimeter_heuristic.h"

#include "../symbolic/sym_variables.h"
#include "../symbolic/uniform_cost_search.h"
#include "../symbolic/original_state_space.h"
#include "../plugins/plugin.h"

#include <cassert>
#include <limits>
#include <memory>

using namespace std;

namespace symbolic {
    SymbolicPerimeterHeuristic::SymbolicPerimeterHeuristic(int generation_time, double generation_memory,
                                                            std::shared_ptr<BDDManager> bdd_manager,
                                                            const std::shared_ptr<variable_ordering::VariableOrderingStrategy> &variable_ordering,
                                                            const SymParamsMgr &mgrParams,
                                                            const SymParamsSearch &searchParams,
                                                            const shared_ptr<AbstractTask> &transform,
                                                            bool cache_estimates,
                                                            const string &description,
                                                            utils::Verbosity verbosity
    )
        : Heuristic(transform, cache_estimates, description, verbosity),
          generation_time(generation_time), generation_memory(generation_memory),
          vars(std::make_shared<SymVariables>(bdd_manager, *variable_ordering, task)) {
        utils::Timer timer;
        log << "Initializing perimeter heuristic..." << endl;

        auto originalStateSpace = make_shared<OriginalStateSpace>(vars.get(), mgrParams, task);
        auto solution = make_shared<SymSolutionLowerBound>();
        auto search = make_unique<UniformCostSearch>(searchParams, originalStateSpace, false, solution);

        while (!search->finished() &&
               (generation_time == 0 || timer() < generation_time) &&
               (generation_memory == 0 || (vars->get_bdd_manager()->totalMemory()) < generation_memory) &&
               !search->solved()) {
            if (!search->step()) break;
        }

        assert(!search->finished() || search->solved());

        if (search->solved()) {
            log << "Problem solved during heuristic generation" << endl;

            //TODO: heuristic = make_unique<ADD>(solution.getADD());
        }

        perimeter_heuristic = make_unique<ADD>(search->getClosed()->getHeuristic());

        log << "Done initializing Perimeter heuristic [" << timer << "] total memory: " << vars->get_bdd_manager()->
                totalMemory() << endl;
    }


    int SymbolicPerimeterHeuristic::compute_heuristic(const State &ancestor_state) {
        State state = convert_ancestor_state(ancestor_state);
        state.unpack();
        int *inputs = vars->getBinaryDescription(state.get_unpacked_values());
        // TODO: for (const BDD &bdd: notMutexBDDs) {
        //     if (bdd.Eval(inputs).IsZero()) {
        //         return DEAD_END;
        //     }
        // }

        ADD evalNode = perimeter_heuristic->Eval(inputs);
        int res = static_cast<int>(Cudd_V(evalNode.getRegularNode()));
        if (res == -1) return DEAD_END;
        else return res;
    }


    class SymbolicPerimeterHeuristicFeature
            : public plugins::TypedFeature<Evaluator, SymbolicPerimeterHeuristic> {
    public:
        SymbolicPerimeterHeuristicFeature() : TypedFeature("sp") {
            document_title("Symbolic Perimeter heuristic");

            add_heuristic_options_to_feature(*this, "sp");
            SymController::add_options_to_feature(*this, utils::Duration(30), 1e7);

            add_option<int>("generation_time", "maximum time used in heuristic generation", "1200");

            add_option<double>("generation_memory",
                               "maximum memory used in heuristic generation", to_string(3e9));


            document_language_support("action costs", "supported");
            document_language_support("conditional effects", "supported");
            document_language_support("axioms", "not supported");

            document_property("admissible", "yes");
            document_property("consistent", "yes");
            document_property("safe", "yes");
            document_property("preferred operators", "no");
        }

        shared_ptr<SymbolicPerimeterHeuristic> create_component(const plugins::Options &opts) const override {
            auto bdd_manager = make_shared<BDDManager>(opts);
            SymParamsMgr mgr_params(opts);
            SymParamsSearch search_params(opts);

            return plugins::make_shared_from_arg_tuples<SymbolicPerimeterHeuristic>(
                opts.get<int>("generation_time"),
                opts.get<double>("generation_memory"),
                bdd_manager,
                opts.get<shared_ptr<variable_ordering::VariableOrderingStrategy> >("variable_ordering"), mgr_params, search_params,
                get_heuristic_arguments_from_options(opts)
            );
        }
    };

    static plugins::FeaturePlugin<SymbolicPerimeterHeuristicFeature> _plugin;
}
