#include "dominance_pruning_lower_g.h"
#include "../plugins/plugin.h"
#include "../utils/markup.h"
#include "../task_proxy.h"
#include "../factored_transition_system/factored_state_mapping.h"

using namespace std;

typedef std::vector<int> ExplicitState;

namespace dominance {

    DominancePruningLowerG::DominancePruningLowerG(const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                                             std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                                             utils::Verbosity verbosity) :
            DominancePruning(fts_factory, dominance_analysis, verbosity) {
        // Removed compare_initial_state and compare_siblings, since we will compare against all states
    }

    void DominancePruningLowerG::initialize(const std::shared_ptr<AbstractTask> &task) {
        DominancePruning::initialize(task);
        // Get the initial state of the task
        TaskProxy task_proxy(*task);
        State initial(task_proxy.get_initial_state());

        // Store the initial state and its transformed state
        // previous_states.push_back(initial.get_unpacked_values());
        ExplicitState initial_state = initial.get_unpacked_values();
        ExplicitState initial_transformed_state = state_mapping->transform(initial_state);

        previous_transformed_states.push_back(state_mapping->transform(initial.get_unpacked_values()));  
        store_state(initial_transformed_state, 0);
    }

    bool DominancePruningLowerG::must_prune_operator(const OperatorProxy & op,
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
            must_prune = compare_against_all_previous_states(succ_transformed, g_value);
        }

        // Store the newly generated state
        if (!must_prune) {
            store_state(succ_transformed, g_value);
        }

        // Reset succ and succ_transformed for the next operator
        succ = parent;
        succ_transformed = parent_transformed;
        return must_prune;
    }



    void DominancePruningLowerG::prune_generation(const State &state, const SearchNodeInfo &node_info, std::vector<OperatorID> &op_ids) {
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


    void DominancePruningLowerG::store_state(const vector<int>& transformed_state, int g_value) {
        // previous_states.push_back(state);
        previous_transformed_states.push_back(transformed_state);
        previous_states_sorted[g_value].emplace_back(transformed_state);
    }


    bool DominancePruningLowerG::compare_against_all_previous_states(const ExplicitState &succ_transformed, int g_value) const {
        // Loop over all previously generated states in the map, ordered by g_value
        for (const auto& [stored_g_value, transformed_states] : previous_states_sorted) {
            // cout << "stored_g_value: " << stored_g_value << endl;
            // Skip states with higher g values
            if (stored_g_value > g_value) {
                break;
            }

            // Compare the current state against all states with the same or lower g values
            for (const ExplicitState &stored_transformed : transformed_states) {
                bool dominates = true;
                for (size_t factor = 0; factor < stored_transformed.size(); ++factor) {
                    if (!(*dominance_relation)[factor].simulates(stored_transformed[factor], succ_transformed[factor])) {
                        dominates = false;
                        break;
                    }
                }

                if (dominates) {
                    return true;  // Prune if dominated by any previous state
                }
            }
        }
        return false;
    }


    // Plugin registration
    class DominancePruningLowerGFeature
            : public plugins::TypedFeature<PruningMethod, DominancePruningLowerG> {
    public:
        DominancePruningLowerGFeature() : TypedFeature("dominance_lower_g") {
            document_title("All Previous Dominance Pruning");

            document_synopsis(
                    "This pruning method implements a strategy where all previously generated states "
                    "are compared against new states to see if the new state is dominated by any previous state.");

            // Removed the options for comparing initial state and siblings
            add_dominance_pruning_options_to_feature(*this);
        }

        virtual shared_ptr<DominancePruningLowerG> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {

            return plugins::make_shared_from_arg_tuples<DominancePruningLowerG>(
                    get_dominance_pruning_arguments_from_options(opts));
        }
    };

    static plugins::FeaturePlugin<DominancePruningLowerGFeature> _plugin;

}
