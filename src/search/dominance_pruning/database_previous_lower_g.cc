#include "database_previous_lower_g.h"

#include "../plugins/plugin.h"

#include "../dominance/state_dominance_relation.h"
namespace dominance {

    void DatabasePreviousLowerG::insert(const ExplicitState &transformed_state, int g_value) {
        previous_states_sorted[g_value].emplace_back(transformed_state);
    }

    bool DatabasePreviousLowerG::check(const ExplicitState &succ_transformed, int g_value) const {
        // Loop over all previously generated states in the map, ordered by g_value
        for (const auto& [stored_g_value, transformed_states] : previous_states_sorted) {
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



    class DatabasePreviousLowerGFactoryFeature
    : public plugins::TypedFeature<DominanceDatabaseFactory, DatabasePreviousLowerGFactory> {
    public:
        DatabasePreviousLowerGFactoryFeature() : TypedFeature("previous_lower_g") {
            document_title("All Previous Dominance Pruning");

            document_synopsis(
                "This pruning method implements a strategy where all previously generated states "
                "are compared against new states to see if the new state is dominated by any previous state.");

            }

            virtual std::shared_ptr<DatabasePreviousLowerGFactory> create_component(const plugins::Options &,
            const utils::Context &) const override {
            return std::make_shared<DatabasePreviousLowerGFactory>();
        }
    };

    static plugins::FeaturePlugin<DatabasePreviousLowerGFactoryFeature> _plugin;


}
