#include "database_all_previous.h"

#include "../dominance/state_dominance_relation.h"
#include "../plugins/plugin.h"

namespace dominance {
    void DatabaseAllPrevious::insert(const std::vector<int>& transformed_state, int) {
        // previous_states.push_back(state);
        previous_transformed_states.push_back(transformed_state);
    }

    bool DatabaseAllPrevious::check(const ExplicitState &succ_transformed, int) const {
        // Loop over all previously generated states and check if any dominate the current state
        for (size_t i = 0; i < previous_transformed_states.size(); ++i) {

            // const ExplicitState &prev_state = previous_states[i];
            const ExplicitState &prev_transformed = previous_transformed_states[i];

            bool dominates = true;
            // Iterate  over each factor of the transformed states. If some factor does not dominate, then the previous state does not dominate the current state
            for (size_t factor = 0; factor < prev_transformed.size(); ++factor) {
                // Check if the previous state dominates the current state for this factor
                if (!(*dominance_relation)[factor].simulates(prev_transformed[factor], succ_transformed[factor])) {
                    dominates = false;
                    break;
                }
            }

            if (dominates) {
                return true;  // Prune if dominated by any previous state
            }
        }
        return false;
    }


    class DatabaseAllPreviousFactoryFeature
: public plugins::TypedFeature<DominanceDatabaseFactory, DatabaseAllPreviousFactory> {
    public:
        DatabaseAllPreviousFactoryFeature() : TypedFeature("all_previous") {
            document_title("All Previous Dominance Pruning");

            document_synopsis(
                "This pruning method implements a strategy where all previously generated states "
                "are compared against new states to see if the new state is dominated by any previous state.");

        }

        virtual std::shared_ptr<DatabaseAllPreviousFactory> create_component(const plugins::Options &) const override {
            return std::make_shared<DatabaseAllPreviousFactory>();
        }
    };

    static plugins::FeaturePlugin<DatabaseAllPreviousFactoryFeature> _plugin;


}
