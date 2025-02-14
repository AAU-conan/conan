#include "label_grouped_label_relation.h"

#include <numeric>
#include <vector>
#include <memory>
#include <print>

#include "label_group_relation.h"
#include "sparse_factor_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../plugins/plugin.h"

using namespace std;
using namespace fts;

namespace dominance {

    LabelGroupedLabelRelation::LabelGroupedLabelRelation(const fts::FTSTask & fts_task) : LabelRelation(fts_task.get_num_labels()) {
        for (size_t i = 0; i < fts_task.get_factors().size(); ++i) {
            const auto& lts = fts_task.get_factor(i);
            label_group_simulation_relations.emplace_back(lts, i);
        }
    }

    bool LabelGroupedLabelRelation::label_group_simulates(int factor, LabelGroup lg1, LabelGroup lg2) const {
        return label_group_simulation_relations.at(factor).simulates(lg1, lg2);
    }

    bool LabelGroupedLabelRelation::noop_simulates_label_group(int factor, LabelGroup lg) const {
        return label_group_simulation_relations.at(factor).noop_simulates(lg);
    }

    bool LabelGroupedLabelRelation::label_dominates_label_in_all_other(const int factor, const fts::FTSTask& fts_task, const int l1,
        const int l2) const {
        return fts_task.get_label_cost(l1) <= fts_task.get_label_cost(l2) && std::ranges::all_of(std::views::iota(0u, fts_task.get_factors().size()), [&](const int& j) {
            const auto& lts = fts_task.get_factor(j);
            return j == factor || label_group_simulates(j, lts.get_group_label(l1), lts.get_group_label(l2));
        });
    }

    bool LabelGroupedLabelRelation::noop_dominates_label_in_all_other(const int factor, const fts::FTSTask& fts_task, const int l) const {
        return std::ranges::all_of(std::views::iota(0u, fts_task.get_factors().size()), [&](const int& j) {
            const auto& lts = fts_task.get_factor(j);
            return j == factor || noop_simulates_label_group(j, lts.get_group_label(l));
        });
    }

    bool LabelGroupedLabelRelation::update_factor(int factor, const fts::FTSTask& /*fts_task*/, const FactorDominanceRelation& sim) {
        return label_group_simulation_relations.at(factor).update(sim);
    }

    void LabelGroupedLabelRelation::print_label_dominance() const {
        for (const auto& lgsr : label_group_simulation_relations) {
            std::println("Factor {}", lgsr.factor);
            for (const auto& [lg1, lg2] : lgsr.simulations) {
                std::println("{} simulates {}", lgsr.lts.label_group_name(lg1), lgsr.lts.label_group_name(lg2));
            }
            for (const auto& lg : lgsr.noop_simulations) {
                std::println("noop simulates {}", lgsr.lts.label_group_name(lg));
            }
            for (const auto& lg : lgsr.simulations_noop) {
                std::println("{} simulates noop", lgsr.lts.label_group_name(lg));
            }
        }
    }

    using LabelGroupedLabelRelationFactory = LabelRelationFactoryImpl<LabelGroupedLabelRelation>;
    class LabelGroupedLabelRelationFactoryFeature final : public plugins::TypedFeature<LabelRelationFactory, LabelGroupedLabelRelationFactory> {
    public:
        LabelGroupedLabelRelationFactoryFeature() : TypedFeature("grouped_lr") {
            document_title("Label Grouped Label Relation");
            document_synopsis("Stores the label relation in separate sparse label group relations for each factor");
        }

        [[nodiscard]] shared_ptr<LabelGroupedLabelRelationFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
            utils::unused_variable(opts);
            return plugins::make_shared_from_arg_tuples<LabelGroupedLabelRelationFactory>();
        }
    };
    static plugins::FeaturePlugin<LabelGroupedLabelRelationFactoryFeature> _grouped_plugin;
}
