#include "qualified_local_state_relation2.h"

#include "../factored_transition_system/labelled_transition_system.h"
#include "../merge_and_shrink/transition_system.h"
#include "../utils/logging.h"
#include "qualified_label_relation.h"
#include "../utils/graphviz.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <ext/slist>
#include <utility>
#include <print>

#include <mata/nfa/nfa.hh>

#include "../factored_transition_system/fts_task.h"


using namespace std;
using merge_and_shrink::TransitionSystem;
using fts::LabelledTransitionSystem;

namespace qdominance {

    int QualifiedLocalStateRelation2::num_equivalences() const {
        return std::ranges::count_if(simulations, [&](const auto p) { return simulations.contains({p.second, p.first}); });
    }

    int QualifiedLocalStateRelation2::num_simulations(bool ignore_equivalences) const {
        return simulations.size();
    }

    int QualifiedLocalStateRelation2::num_different_states() const {
        int num = 0;
        std::vector<bool> counted(simulations.size(), false);
        for (size_t i = 0; i < counted.size(); i++) {
            if (!counted[i]) {
                num++;
                for (size_t j = i + 1; j < simulations.size(); j++) {
                    if (similar(i, j)) {
                        counted[j] = true;
                    }
                }
            }
        }
        return num;
    }

//Computes the probability of selecting a random pair s, s' such
//that s simulates s'.
    double QualifiedLocalStateRelation2::get_percentage_simulations(bool ignore_equivalences) const {
        double num_sims = num_simulations(ignore_equivalences);
        double num_states = (ignore_equivalences ? num_different_states() : simulations.size());
        return num_sims / (num_states * num_states);
    }

//Computes the probability of selecting a random pair s, s' such that
//s is equivalent to s'.
    double QualifiedLocalStateRelation2::get_percentage_equivalences() const {
        double num_eq = 0;
        double num_states = simulations.size();
        for (size_t i = 0; i < simulations.size(); ++i)
            for (size_t j = 0; j < simulations.size(); ++j)
                if (similar(i, j))
                    num_eq++;
        return num_eq / (num_states * num_states);
    }

    void QualifiedLocalStateRelation2::compute_list_dominated_states() {
        dominated_states.resize(simulations.size());
        dominating_states.resize(simulations.size());

        for (size_t s = 0; s < simulations.size(); ++s) {
            for (size_t t = 0; t < simulations.size(); ++t) {
                if (simulates(t, s)) {
                    dominated_states[t].push_back(s);
                    dominating_states[s].push_back(t);
                }
            }
        }
    }

    void QualifiedLocalStateRelation2::cancel_simulation_computation() {
    }

    bool QualifiedLocalStateRelation2::update(const QualifiedLabelRelation& label_relation) {
        bool any_changes = false;
        bool changes = false;
        do {
            changes = update_pairs(label_relation);
            any_changes |= changes;
        } while (changes);
#ifndef NDEBUG
        // std::println("{} simulations: {}", factor, boost::algorithm::join(std::views::transform(simulations, [&](const auto& p) { return std::format("{} <= {}", lts.state_name(p.second), lts.state_name(p.first));}) | std::ranges::to<std::vector>(), ", "));
#endif
        return any_changes;
    }

    bool QualifiedLocalStateRelation2::noop_simulates_tr(int t, LTSTransition s_tr, const QualifiedLabelRelation& label_relation) const {
        return label_relation.noop_simulates_label_group_in_all_other(factor, s_tr.label_group) && simulates(t, s_tr.target);
    }

    bool QualifiedLocalStateRelation2::tr_simulates_tr(const LTSTransition& t_tr, const LTSTransition& s_tr, const QualifiedLabelRelation& label_relation) const {
        return label_relation.label_group_simulates_label_group_in_all_other(factor, t_tr.label_group, s_tr.label_group) && simulates(t_tr.target, s_tr.target);
    }

    bool QualifiedLocalStateRelation2::labels_simulate_labels(const std::unordered_set<int>& l1s, const std::vector<int>& l2s, bool include_noop, const QualifiedLabelRelation& label_relation) {
        return std::ranges::all_of(l2s, [&](const auto& l2) {
            return (include_noop && label_relation.noop_simulates_label_in_all_other(factor, l2)) || std::ranges::any_of(l1s, [&](const auto& l1) {
                return label_relation.label_simulates_label_in_all_other(factor, l1, l2);
            });
        });
    }

    // Update the simulation relation for when t simulates s
    bool QualifiedLocalStateRelation2::update_pairs(const QualifiedLabelRelation& label_relation) {
        // Remove all simulation pairs (s, t) where it is not the case that
        // ∀s -lg-> s'( ∃t -lg'-> t' s.t. s' simulates t' and lg' simulates lg in all other factors or t simulates s' and noop simulates lg in all other factors)
        return 0 < erase_if(simulations, [&](const auto p) {
            const auto [t, s] = p;
#ifndef NDEBUG
            // std::println("Checking {} <= {}", lts.state_name(s), lts.state_name(t));
#endif
            const auto& s_transitions = lts.get_transitions(s);
            const auto& t_transitions = lts.get_transitions(t);
            return !std::ranges::all_of(s_transitions, [&](const LTSTransition& s_tr) {
#ifndef NDEBUG
                // std::println("    {} --{}-> {}", lts.state_name(s_tr.src), lts.label_group_name(s_tr.label_group), lts.state_name(s_tr.target));
#endif
                if (!lts.is_relevant_label_group(s_tr.label_group)) {
                    return true;
                }

                std::unordered_set<int> t_labels;
                for (const LTSTransition& t_tr : t_transitions) {
                    if (simulates(t_tr.target, s_tr.target)) {
                        for (const auto& l : lts.get_labels(t_tr.label_group)) {
                            t_labels.insert(l);
                        }
                    }
                }

                return labels_simulate_labels(t_labels, lts.get_labels(s_tr.label_group), simulates(t, s_tr.target), label_relation);
#ifndef NDEBUG
                // std::println("        {0} --noop-> {0} does {1}simulate", lts.state_name(t), noop_simulates_tr(t, s_tr, label_relation)? "" : "not ");
#endif
            });
        });
    }

    QualifiedLocalStateRelation2::QualifiedLocalStateRelation2(const LabelledTransitionSystem& lts, int factor, const FTSTask& fts_task)
        : lts(lts), factor(factor), fts_task(fts_task) {
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
}
