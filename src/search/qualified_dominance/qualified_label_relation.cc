#include "qualified_label_relation.h"

#include <numeric>
#include <vector>
#include <memory>
#include <print>

#include "label_group_relation.h"
#include "qualified_local_state_relation2.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"

using namespace std;
using namespace fts;

namespace qdominance {

    QualifiedLabelRelation::QualifiedLabelRelation(const fts::FTSTask & fts_task)
    : fts_task(fts_task) {
        int num_factors = fts_task.get_num_variables();
        const auto num_labels = fts_task.get_num_labels();

        simulates_irrelevant.resize(num_factors);
        simulated_by_irrelevant.resize(num_factors);

        dominates_in.resize(num_labels);
        dominated_by_noop_in.resize(num_labels, AllNoneFactorIndex::all_factors());
        for (size_t l1 = 0; l1 < num_labels; ++l1) {
            for (size_t l2 = 0; l2 < num_labels; ++l2) {
                if (fts_task.get_label_cost(l1) <= fts_task.get_label_cost(l2)) {
                    dominates_in[l1].emplace(l2, AllNoneFactorIndex::all_factors());
                }
            }
        }

        label_state_transition_map.resize(fts_task.get_factors().size());
        for (int i = 0; i < fts_task.get_factors().size(); ++i) {
            const auto& lts = fts_task.get_factor(i);

            std::set<LabelGroup> all_label_groups = std::views::iota(0, lts.get_num_label_groups()) | std::views::transform([&](int i) { return LabelGroup(i); }) | ranges::to<std::set<LabelGroup>>();
            simulated_by_irrelevant.at(i) = all_label_groups;
            simulates_irrelevant.at(i) = all_label_groups;

            label_group_simulation_relations.emplace_back(lts, i);
            label_state_transition_map[i].resize(lts.get_num_label_groups());

            for (const auto [tr_i, tr] : std::views::enumerate(lts.get_transitions())) {
                auto& state_transitions = label_state_transition_map[i][tr.label_group.group];
                state_transitions.try_emplace(tr.src); // Create vector for transition if it does not already exist
                state_transitions.at(tr.src).push_back(tr_i);
            }
        }
    }


    void QualifiedLabelRelation::dump_dominance(utils::LogProxy &) const {
        //TODO: implement
/*
        for (size_t l1 = 0; l1 < dominates_in.size(); ++l1) {
            for (size_t l2 = 0; l2 < dominates_in.size(); ++l2) {
                if (!dominates_in[l1][l2].is_none() && dominates_in[l2][l1] != dominates_in[l1][l2]) {
                   // log << l1 << " dominates " << l2 << " in " << dominates_in[l1][l2] << endl;
//                    log << g_operators[l1].get_name() << " dominates " << g_operators[l2].get_name() << endl;
                }
            }
        }
*/
    }

    bool QualifiedLabelRelation::label_group_simulates(int factor, LabelGroup lg1, LabelGroup lg2) const {
        return label_group_simulation_relations.at(factor).simulates(lg1, lg2, *this);
    }

    bool QualifiedLabelRelation::label_group_simulated_by_noop(int factor, LabelGroup lg) const {
        return label_group_simulation_relations.at(factor).simulated_by_noop(lg, *this);
    }

    bool QualifiedLabelRelation::update_label_pair(int l1, int l2, int factor, const QualifiedLocalStateRelation2 &sim) {
        // Check if l1 simulates l2 in lts
        // For each transition s -l1--> s', check if there is a transition s -l2-> s'' in lts such that s'' simulates s'
        const auto &lts = fts_task.get_factor(factor);
        LabelGroup lg1 = lts.get_group_label(l1);
        for (const auto &tr: lts.get_transitions_label(l2)) {
            if (!std::ranges::any_of(transitions_for_label_group_state(factor, lg1, tr.src, lts),
                [&](const auto& tr2) { return sim.simulates(tr2.target, tr.target); }))
            {
                set_not_simulates(l1, l2, factor);
                return true;
            }
        }
        return false;
    }

    bool QualifiedLabelRelation::update_irrelevant(int factor, const QualifiedLocalStateRelation2 &sim) {
        const auto &lts = fts_task.get_factor(factor);
        bool changes = false;
        // For every label group that is simulated by irrelevant labels, check if it is still simulated
        // A label l is simulated by irrelevant if for each transition s -l-> s', there is a transition s -l'-> s, where
        // l' is an irrelevant label, and s simulates s'. I.e., the l-transition is worse than just staying in s.
        changes |= 0 < std::erase_if(simulated_by_irrelevant.at(factor), [&](const LabelGroup lg) {
            for (const auto& tr : lts.get_transitions_label_group(lg)) {
                if (!sim.simulates(tr.src, tr.target)) {
                    // If src does not simulate target, then an irrelevant label cannot simulate
                    for (const int l2 : lts.get_labels(lg)) {
                        set_not_simulated_by_noop(l2, factor);
                        for (const int l: lts.get_irrelevant_labels()) {
                            if (simulates(l, l2, factor)) {
                                set_not_simulates(l, lg.group, factor);
                            }
                        }
                    }

                    return true; // Remove this label group
                }
            }
            return false; // Do not remove this label group
        });

        // For every label group that simulates irrelevant labels, check if it still simulates
        // A label l simulates irrelevant if for each transition s -l'-> s with irrelevant label l', there is a
        // transition s -l-> s', and s' simulates s. I.e., the l-transition is better than just staying in s.
        changes |= 0 < std::erase_if(simulates_irrelevant.at(factor), [&](const LabelGroup lg) {
            for (int s = 0; s < lts.size(); s++) {
                if (!std::ranges::any_of(transitions_for_label_group_state(factor, lg, s, lts), [&](const auto& tr) {
                        return sim.simulates(tr.target, s);
                    })) {
                    for (const int li: lts.get_irrelevant_labels()) {
                        for (const int l2 : lts.get_labels(lg)) {
                            if (simulates(l2, li, factor)) {
                                set_not_simulates(l2, li, factor);
                            }
                        }
                    }
                    return true; // Remove this label group
                }
            }
            return false; // Do not remove this label group
        });
        return changes;
    }

    bool QualifiedLabelRelation::update(std::vector<std::unique_ptr<QualifiedLocalStateRelation2>> &sims) {
        bool changes = false;
        for (int l1 = 0; l1 < fts_task.get_num_labels(); ++l1) {
            auto l1_dominates = dominates_in[l1]; // Copy to avoid invalidating iterators. TODO: Avoid copying
            for (const auto& [l2, factors] : l1_dominates) {
                if (factors.is_all()) {
                    for (int factor = 0; factor < fts_task.get_num_variables(); ++factor) {
                        update_label_pair(l1, l2, factor, *sims[factor]);
                    }
                } else if (factors.is_factor()) {
                    const auto factor = factors.get_not_present_factor();
                    update_label_pair(l1, l2, factor, *sims[factor]);
                }
            }
        }

        for (int factor = 0; factor < fts_task.get_num_variables(); ++factor) {
            changes |= update_irrelevant(factor, *sims[factor]);
        }

        return changes;
    }

    void QualifiedLabelRelation::set_not_simulates(int l1, int l2, int factor) {
        // Figure out which caches to invalidate
        if (dominates_in[l1].contains(l2)) {
            if (dominates_in[l1].at(l2).is_all()) {
                // If it currently dominates in all, invalidate all caches other than factor
                for (int i = 0; i < label_group_simulation_relations.size(); ++i) {
                    if (i != factor) {
                        label_group_simulation_relations[i].invalidate_label_cache(l1, l2);
                    }
                }
                dominates_in[l1].at(l2).remove(factor);
            } else if (dominates_in[l1].at(l2).is_factor()) {
                // There is one factor in which it currently dominates, invalidate the cache for that factor
                label_group_simulation_relations[dominates_in[l1].at(l2).get_not_present_factor()].invalidate_label_cache(l1, l2);
                dominates_in[l1].erase(l2);
            }
        }
    }

    void QualifiedLabelRelation::set_not_simulated_by_noop(int l, int factor) {
        // Figure out which caches to invalidate
        if (dominated_by_noop_in[l].is_all()) {
            // If it currently dominates in all, invalidate all caches other than factor
            for (int i = 0; i < label_group_simulation_relations.size(); ++i) {
                if (i != factor) {
                    label_group_simulation_relations[i].invalidate_noop_cache(l);
                }
            }
        } else if (dominated_by_noop_in[l].is_factor()) {
            // There is one factor in which it currently dominates, invalidate the cache for that factor
            label_group_simulation_relations[dominated_by_noop_in[l].get_not_present_factor()].invalidate_noop_cache(l);
        }
        dominated_by_noop_in[l].remove(factor);
    }
}
