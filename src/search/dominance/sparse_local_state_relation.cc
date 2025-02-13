#include "sparse_local_state_relation.h"

#include "../factored_transition_system/labelled_transition_system.h"
#include "../merge_and_shrink/transition_system.h"
#include "../utils/logging.h"
#include "../plugins/plugin.h"

#include <utility>

using namespace std;
using merge_and_shrink::TransitionSystem;
using fts::LabelledTransitionSystem;

namespace dominance {
    int SparseLocalStateRelation::num_simulations() const {
        return simulations.size();
    }

    void SparseLocalStateRelation::cancel_simulation_computation() {
        simulations.clear();
    }

    SparseLocalStateRelation::SparseLocalStateRelation(const LabelledTransitionSystem& lts) : FactorDominanceRelation(lts.size()) {
        // Add all pairs that satisfy the goal condition
        for (int s = 0; s < lts.size(); ++s) {
            for (int t = 0; t < lts.size(); ++t) {
                if (!lts.is_goal(s) || lts.is_goal(t)) {
                    // Only add t-transition responses if goal(s) => goal(t)
                    simulations.emplace(t, s);
                }
            }
        }
    }

    bool SparseLocalStateRelation::simulates(int t, int s) const {
        return s == t || simulations.contains({t, s});
    }

    bool SparseLocalStateRelation::similar(int s, int t) const {
        return simulates(s, t) && simulates(t, s);
    }

    bool SparseLocalStateRelation::apply_to_simulations_until(std::function<bool(int s, int t)>&& f) const {
        for (const auto& [s, t] : simulations) {
            if (f(s, t))
                return true;
        }
        return false;
    }

    bool SparseLocalStateRelation::remove_simulations_if(std::function<bool(int s, int t)>&& f) {
        return 0 < erase_if(simulations, [f](std::pair<int,int> p) { return f(p.first, p.second); });
    }

    using SparseLocalStateRelationFactory = FactorDominanceRelationFactoryImpl<SparseLocalStateRelation>;
    class SparseLocalStateRelationFactoryFeature final : public plugins::TypedFeature<FactorDominanceRelationFactory, SparseLocalStateRelationFactory> {
    public:
        SparseLocalStateRelationFactoryFeature() : TypedFeature("sparse_fdr") {
            document_title("Sparse Factor Dominance Relation");
            document_synopsis("Stores the simulation relation between states in a hash map");
        }

        [[nodiscard]] shared_ptr<SparseLocalStateRelationFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
            return plugins::make_shared_from_arg_tuples<SparseLocalStateRelationFactory>();
        }
    };
    static plugins::FeaturePlugin<SparseLocalStateRelationFactoryFeature> _sparse_plugin;
}
