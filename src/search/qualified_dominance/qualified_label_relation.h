#ifndef QUALIFIED_LABEL_RELATION_H
#define QUALIFIED_LABEL_RELATION_H

#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <memory>
#include <generator>
#include <print>

#include "../dominance/all_none_factor_index.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "label_group_relation.h"


using namespace dominance;

namespace fts {
    class FTSTask;
    class LabelledTransitionSystem;
    class LabelMap;
}

namespace utils {
    class LogProxy;
}

namespace qdominance {
    using TransitionIndex = size_t;
    class QualifiedLocalStateRelation;

    /*
     * Label relation represents the preorder relations on labels that
     * occur in a set of LTS
     */
    class QualifiedLabelRelation {
        const fts::FTSTask &fts_task;

        std::vector<LabelGroupSimulationRelation> label_group_simulation_relations;

    public:
        explicit QualifiedLabelRelation(const fts::FTSTask& fts_task);

        bool update(int factor, const QualifiedLocalStateRelation& sim);

        void print_label_dominance() const;

        [[nodiscard]] bool label_group_simulates(int factor, fts::LabelGroup lg1, fts::LabelGroup lg2) const;
        [[nodiscard]] bool noop_simulates_label_group(int factor, fts::LabelGroup lg) const;


        [[nodiscard]] int get_num_labels() const {
            return fts_task.get_num_labels();
        }

        // ∀l2 ∈ lg2. ∃l1 ∈ lg1. ∀i != factor. l1 simulates l2 in factor i
        [[nodiscard]] bool label_group_simulates_label_group_in_all_other(const int factor, const LabelGroup lg1, const LabelGroup lg2) const {
            auto res = std::ranges::all_of(fts_task.get_factor(factor).get_labels(lg2), [&](const int l2) {
                return std::ranges::any_of(fts_task.get_factor(factor).get_labels(lg1), [&](const int l1) {
                    return std::ranges::all_of(std::views::iota(0u, fts_task.get_factors().size()), [&](const unsigned long& i) {
                        const auto& lts = fts_task.get_factor(i);
                        return i == factor || (label_group_simulates(i, lts.get_group_label(l1), lts.get_group_label(l2)) && fts_task.get_label_cost(l1) <= fts_task.get_label_cost(l2));
                    });
                });
            });
#ifndef NDEBUG
            // std::println("{} {}simulates {} in all other {}", fts_task.get_factor(factor).label_group_name(lg1), res ? "" : "not ", fts_task.get_factor(factor).label_group_name(lg2), factor);
#endif
            return res;
        }

        // ∀l ∈ lg. ∀i != factor. noop simulates l in factor i
        [[nodiscard]] bool noop_simulates_label_group_in_all_other(const int factor, const LabelGroup lg) const {
            auto res = std::ranges::all_of(fts_task.get_factor(factor).get_labels(lg), [&](const int l) {
                return std::ranges::all_of(std::views::iota(0u, fts_task.get_factors().size()), [&](const unsigned long& i) {
                    const auto& lts = fts_task.get_factor(i);
                    return i == factor || noop_simulates_label_group(i, lts.get_group_label(l));
                });
            });
            return res;
        }

        [[nodiscard]] bool label_simulates_label_in_all_other(const int factor, const int l1, const int l2) const {
            return fts_task.get_label_cost(l1) <= fts_task.get_label_cost(l2) && std::ranges::all_of(std::views::iota(0u, fts_task.get_factors().size()), [&](const unsigned long& j) {
                const auto& lts = fts_task.get_factor(j);
                return j == factor || label_group_simulates(j, lts.get_group_label(l1), lts.get_group_label(l2));
            });
        }

        [[nodiscard]] bool noop_simulates_label_in_all_other(const int factor, const int l) const {
            return std::ranges::all_of(std::views::iota(0u, fts_task.get_factors().size()), [&](const unsigned long& j) {
                const auto& lts = fts_task.get_factor(j);
                return j == factor || noop_simulates_label_group(j, lts.get_group_label(l));
            });
        }
    };
}


#endif
