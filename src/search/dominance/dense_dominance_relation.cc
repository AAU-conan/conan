#include "dense_dominance_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../plugins/plugin.h"

namespace dominance {
 bool DenseDominanceRelation::apply_to_simulations_until(std::function<bool(int s, int t)>&& f) const {
        for (int s = 0; s < static_cast<int>(relation.size()); ++s) {
            for (int t = 0; t < static_cast<int>(relation.size()); ++t) {
                if (simulates(s, t) && f(s, t)) {
                    return true;
                }
            }
        }
        return false;
    }

    bool DenseDominanceRelation::remove_simulations_if(std::function<bool(int s, int t)>&& f) {
        bool any = false;
        for (int s = 0; s < static_cast<int>(relation.size()); ++s) {
            for (int t = 0; t < static_cast<int>(relation.size()); ++t) {
                if (s != t && simulates(s, t) && f(s, t)) {
                    relation[s][t] = false;
                    any = true;
                }
            }
        }
        return any;
    }


    void DenseDominanceRelation::compute_list_dominated_states() {
        dominated_states.resize(relation.size());
        dominating_states.resize(relation.size());

        for (size_t s = 0; s < relation.size(); ++s) {
            for (size_t t = 0; t < relation.size(); ++t) {
                if (simulates(t, s)) {
                    dominated_states[t].push_back(s);
                    dominating_states[s].push_back(t);
                }
            }
        }
    }

    void DenseDominanceRelation::cancel_simulation_computation() {
        std::vector<std::vector<bool> >().swap(relation);
    }

    DenseDominanceRelation::DenseDominanceRelation(const fts::LabelledTransitionSystem& lts) : FactorDominanceRelation(lts.size()),
        relation(lts.size(), std::vector<bool>(lts.size(), true)) {
        int num_states = lts.size();
        const std::vector<bool> &goal_states = lts.get_goal_states();
        const std::vector<int> &goal_distances = lts.get_goal_distances();
        relation.resize(num_states);
        for (int i = 0; i < num_states; i++) {
            relation[i].resize(num_states, true);
            if (!goal_states[i]) {
                for (int j = 0; j < num_states; j++) {
                    //TODO (efficiency): initialize with goal distances
                    if (goal_states[j] /*|| goal_distances[i] > goal_distances[j]*/) {
                        relation[i][j] = false;
                    }
                }
            }
        }
    }


    using DenseLocalStateRelationFactory = FactorDominanceRelationFactoryImpl<DenseDominanceRelation>;
    class DenseLocalStateRelationFactoryFeature final : public plugins::TypedFeature<FactorDominanceRelationFactory, DenseLocalStateRelationFactory> {
    public:
        DenseLocalStateRelationFactoryFeature() : TypedFeature("dense_fdr") {
            document_title("Dense Factor Dominance Relation");
            document_synopsis("Stores the simulation relation between states in a dense matrix");
        }

        [[nodiscard]] std::shared_ptr<DenseLocalStateRelationFactory> create_component(
        const plugins::Options &/*opts*/,
        const utils::Context &) const override {
            return plugins::make_shared_from_arg_tuples<DenseLocalStateRelationFactory>();
        }
    };
    static plugins::FeaturePlugin<DenseLocalStateRelationFactoryFeature> _dense_plugin;
}