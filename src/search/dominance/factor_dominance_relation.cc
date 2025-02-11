#include "factor_dominance_relation.h"

#include "label_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../merge_and_shrink/transition_system.h"
#include "../utils/logging.h"
#include "../plugins/plugin.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <ext/slist>

using namespace std;
using merge_and_shrink::TransitionSystem;
using fts::LabelledTransitionSystem;

namespace dominance {

    [[nodiscard]] bool FactorDominanceRelation::is_identity() const {
        for (int i = 0; i < lts.size(); ++i) {
            for (int j = i + 1; j < lts.size(); ++j) {
                if (simulates(i, j) || simulates(j, i)) {
                    return false;
                }
            }
        }
        return true;
    }

    void FactorDominanceRelation::dump(utils::LogProxy& log) const {
        log << "SIMREL:" << std::endl;
        for (int j = 0; j < lts.size(); ++j) {
            for (int i = 0; i < lts.size(); ++i) {
                if (simulates(j, i) && i != j) {
                    if (simulates(i, j)) {
                        if (j < i) {
                            log << lts.state_name(i) << " <=> " << lts.state_name(j) << std::endl;
                        }
                    } else {
                        log << lts.state_name(i) << " <= " << lts.state_name(j) << std::endl;
                    }
                }
            }
        }
    }

    int FactorDominanceRelation::num_equivalences() const {
        int num = 0;
        for (int i = 0; i < static_cast<int>(lts.size()); i++) {
            for (int j = i + 1; j < lts.size(); j++) {
                if (similar(i, j)) {
                    num++;
                }
            }
        }
        return num;
    }

    int FactorDominanceRelation::num_simulations() const {
        int res = 0;
        std::vector<bool> counted(lts.size(), false);
        for (int i = 0; i < lts.size(); ++i) {
            if (!counted[i]) {
                for (int j = i + 1; j < lts.size(); ++j) {
                    if (similar(i, j)) {
                        counted[j] = true;
                    }
                }
            }
        }
        for (int i = 0; i < lts.size(); ++i) {
            if (!counted[i]) {
                for (int j = i + 1; j < lts.size(); ++j) {
                    if (!counted[j]) {
                        if (!similar(i, j) && (simulates(i, j) || simulates(j, i))) {
                            res++;
                        }
                    }
                }
            }
        }
        return res;
    }

    int FactorDominanceRelation::num_different_states() const {
        int num = 0;
        std::vector<bool> counted(lts.size(), false);
        for (int i = 0; i < static_cast<int>(counted.size()); i++) {
            if (!counted[i]) {
                num++;
                for (int j = i + 1; j < lts.size(); j++) {
                    if (similar(i, j)) {
                        counted[j] = true;
                    }
                }
            }
        }
        return num;
    }

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
        vector<vector<bool> >().swap(relation);
    }

    DenseDominanceRelation::DenseDominanceRelation(const LabelledTransitionSystem& lts) : FactorDominanceRelation(lts),
        relation(lts.size(), vector<bool>(lts.size(), true)) {
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

    static class FactorDominanceRelationFactoryPlugin final : public plugins::TypedCategoryPlugin<FactorDominanceRelationFactory> {
    public:
        FactorDominanceRelationFactoryPlugin() : TypedCategoryPlugin("FactorDominanceRelationFactory") {
            document_synopsis( "A FactorDominanceRelationFactory creates FactorDominanceRelations for a given LTS.");
        }
    }
    _category_plugin;

    using DenseLocalStateRelationFactory = FactorDominanceRelationFactoryImpl<DenseDominanceRelation>;
    class DenseLocalStateRelationFactoryFeature final : public plugins::TypedFeature<FactorDominanceRelationFactory, DenseLocalStateRelationFactory> {
    public:
        DenseLocalStateRelationFactoryFeature() : TypedFeature("dense_fdr") {
            document_title("Dense Factor Dominance Relation");
            document_synopsis("Stores the simulation relation between states in a dense matrix");
        }

        [[nodiscard]] shared_ptr<DenseLocalStateRelationFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
            return plugins::make_shared_from_arg_tuples<DenseLocalStateRelationFactory>();
        }
    };
    static plugins::FeaturePlugin<DenseLocalStateRelationFactoryFeature> _dense_plugin;

}
