#include "dominance_pruning_previous.h"
#include "../plugins/plugin.h"
#include "../utils/markup.h"
#include "../task_proxy.h"
#include "../factored_transition_system/factored_state_mapping.h"

using namespace std;

typedef std::vector<int> ExplicitState;

namespace dominance {

    /////////////////////////////////////////////
    //// DOMINANCE PRUNNING PREVIOUS STATES /////
    /////////////////////////////////////////////

    DominancePruningPrevious::DominancePruningPrevious(std::shared_ptr<DominanceDatabaseFactory> database_factory,
    const std::shared_ptr<fts::FTSTaskFactory> &fts_factory,
    std::shared_ptr<DominanceAnalysis> dominance_analysis,
    utils::Verbosity verbosity)
    : DominancePruning(fts_factory, dominance_analysis, verbosity), database_factory(database_factory) {
    }

    void DominancePruningPrevious::initialize(const std::shared_ptr<AbstractTask> &task) {
        DominancePruning::initialize(task);
        // Get the initial state of the task
        TaskProxy task_proxy(*task);
        State initial(task_proxy.get_initial_state());

        database = database_factory->create(task, dominance_relation, state_mapping);
        database->insert(state_mapping->transform(initial.get_unpacked_values()), 0);

        log << "Dominance pruning initialized" << endl;
    }

    void DominancePruningPrevious::prune_generation(const State &state, const SearchNodeInfo &node_info, std::vector<OperatorID> &op_ids) {
        state.unpack();
        const vector<int> & parent = state.get_unpacked_values();
        vector<int> parent_transformed = state_mapping->transform(parent);

        vector<int> succ = parent;
        vector<int> succ_transformed = parent_transformed;

        TaskProxy tp (*task);

        const auto [first, last] = std::ranges::remove_if(op_ids,
                                                          [&] (const auto & op_id) {
            return must_prune_operator(tp.get_operators()[op_id], state, parent, parent_transformed, succ, succ_transformed, node_info.g);
        });

        op_ids.erase(first, last);
    }

    bool DominancePruningPrevious::must_prune_operator(const OperatorProxy & op,
                                                          const State & state,
                                                          const ExplicitState & parent,
                                                          const ExplicitState & parent_transformed,
                                                          ExplicitState & succ,
                                                          ExplicitState & succ_transformed,
                                                          int g_value)  {
        ExplicitState updated_variables; // Vector that will keep track of the variables in the state that have been updated by the current operator
        // Iterate through all effects of the operator op, applies those that fire and updates the succ state and updated_variables
        for (EffectProxy effect : op.get_effects()) {
            if (does_fire(effect, state)) {
                FactPair effect_fact = effect.get_fact().get_pair(); // retrieves the fact associated with the effect (a pairavar-value)
                succ[effect_fact.var] = effect_fact.value; //  updates the succ state at the position of the variable effect_fact.var with the new value effect_fact.value
                updated_variables.push_back(effect_fact.var);
            }
        }
        assert(state_mapping);
        //List of updated variables that changed. To determine which parts (factors) of the transformed state were affected and should be checked for dominance?
        auto maybe_affected_factors = state_mapping->update_transformation_in_place(succ_transformed, succ, updated_variables);

        bool must_prune = false;
        if (!maybe_affected_factors.has_value()) {
            must_prune = true;
        } else {
            // Check if the new state is dominated by the parent state
            // Returns true if all factors are dominated by the parent state
            must_prune = std::ranges::all_of(maybe_affected_factors.value(),
                                             [&](const auto & factor) {
                                                 return (*dominance_relation)[factor].simulates(parent_transformed[factor],
                                                                                                succ_transformed[factor]);
                                             });
        }

        // If the state was not pruned by the parent, compare it against all previous states
        if (!must_prune) {
            must_prune = database->check(succ_transformed, g_value + op.get_cost());
        }

        // Store the newly generated state
        if (!must_prune) {
            database->insert(succ_transformed, g_value + op.get_cost());
        }

        // Reset succ and succ_transformed for the next operator
        succ = parent;
        succ_transformed = parent_transformed;
        return must_prune;
    }


    //////////////////////////////////////////////////
    ////////////// Plugin registration ///////////////
    //////////////////////////////////////////////////

    class DominancePruningAllPreviousFeature
            : public plugins::TypedFeature<PruningMethod, DominancePruningPrevious> {
    public:
        DominancePruningAllPreviousFeature() : TypedFeature("dominance") {
            document_title("All Previous Dominance Pruning");

            document_synopsis(
                    "This pruning method implements a strategy where all previously generated states "
                    "are compared against new states to see if the new state is dominated by any previous state.");

            // Removed the options for comparing initial state and siblings
            add_dominance_pruning_options_to_feature(*this);
            add_option<std::shared_ptr<DominanceDatabaseFactory>>("database", "Database to store the explored states", "bdd_map()");
        }

        virtual shared_ptr<DominancePruningPrevious> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {

            return plugins::make_shared_from_arg_tuples<DominancePruningPrevious>(
            opts.get<std::shared_ptr<DominanceDatabaseFactory>>("database"),
                    get_dominance_pruning_arguments_from_options(opts));
        }
    };

    static plugins::FeaturePlugin<DominancePruningAllPreviousFeature> _plugin;

}
