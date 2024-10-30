#include "label_group_relation.h"
#include "qualified_label_relation.h"

namespace qdominance {
    bool LabelGroupSimulationRelation::label_group_simulates_compute(fts::LabelGroup lg1, fts::LabelGroup lg2, const QualifiedLabelRelation& label_relation) const {
        // ∀l2 ∈ lg2. ∃l1 ∈ lg1. l1 simulates l2
        for (const auto& l2 : lts.get_labels(lg2)) {
            for (const auto& l1 : lts.get_labels(lg1)) {
                if (label_relation.simulates(l1, l2, factor)) {
                    goto dominates;
                }
            }
            return false; // No label of lg1 simulates l2
            dominates:;
        }
        return true;
    }

    bool LabelGroupSimulationRelation::label_group_simulated_by_noop_compute(fts::LabelGroup lg, const QualifiedLabelRelation& label_relation) const {
        // ∀l ∈ lg. l is simulated by noop
        return std::ranges::all_of(lts.get_labels(lg), [&](const auto& l) {
            return label_relation.dominated_by_noop(l, factor);
        });
    }
}

