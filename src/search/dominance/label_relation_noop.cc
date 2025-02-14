#include "label_relation_noop.h"

#include "all_none_factor_index.h"
#include "factor_dominance_relation.h"
#include "state_dominance_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../plugins/plugin.h"

using namespace std;

namespace dominance {

    bool NoopLabelRelation::set_not_simulated_by_irrelevant(int l, int lts) {
        //Returns if there were changes in dominated_by_noop_in
        return dominated_by_noop_in[l].remove(lts);
    }

    bool NoopLabelRelation::update_factor(int factor, const fts::FTSTask& fts_task, const FactorDominanceRelation& sim) {
        bool changes = false;
        const fts::LabelledTransitionSystem& lts = fts_task.get_factor(factor);
        for (fts::LabelGroup lg: lts.get_relevant_label_groups()) {
            for (int l : lts.get_labels(lg)) {
                //Is l2 simulated by irrelevant_labels in lts?
                for (auto tr: lts.get_transitions_label(l)) {
                    if (dominated_by_noop_in[l].contains(factor) && !sim.simulates(tr.src, tr.target)) {
                        changes |= set_not_simulated_by_irrelevant(l, factor);
                        break;
                    }
                }
            }
        }

        return changes;
    }

    NoopLabelRelation::NoopLabelRelation(const fts::FTSTask& fts_task) : LabelRelation(fts_task.get_num_labels()) {
        dominated_by_noop_in.resize(num_labels, AllNoneFactorIndex::all_factors());
    }

    bool NoopLabelRelation::label_dominates_label_in_all_other(int /*factor*/, const fts::FTSTask& /*fts_task*/, int l1, int l2) const {
        return l1 == l2;
    }

    bool NoopLabelRelation::noop_dominates_label_in_all_other(int factor, const fts::FTSTask& /*fts_task*/, int l) const {
        return dominated_by_noop_in[l].contains_all_except(factor);
    }

    using NoopLabelRelationFactory = LabelRelationFactoryImpl<NoopLabelRelation>;
    class NoopLabelRelationFactoryFeature final : public plugins::TypedFeature<LabelRelationFactory, NoopLabelRelationFactory> {
    public:
        NoopLabelRelationFactoryFeature() : TypedFeature("noop_lr") {
            document_title("NOOP Label Relation");
            document_synopsis("Stores label relation for NOOP dominating label in dense vector, and used identity check for all other labels");
        }

        [[nodiscard]] std::shared_ptr<NoopLabelRelationFactory> create_component(
        const plugins::Options &opts,
        const utils::Context &) const override {
            utils::unused_variable(opts);
            return plugins::make_shared_from_arg_tuples<NoopLabelRelationFactory>();
        }
    };
    static plugins::FeaturePlugin<NoopLabelRelationFactoryFeature> _plugin;
}
