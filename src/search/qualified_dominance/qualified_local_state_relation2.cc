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
        int num = 0;
        for (int s = 0; s < simulations.size(); ++s) {
            for (int t : simulations.at(s)) {
                if (simulates(t, s) && s < t) {
                    // Only count each equivalence once
                    ++num;
                }
            }
        }
        return num;
    }

    int QualifiedLocalStateRelation2::num_simulations(bool ignore_equivalences) const {
        int res = 0;
        for (int s = 0; s < simulations.size(); ++s) {
            for (int t : simulations.at(s)) {
                if (!simulates(t, s) || !ignore_equivalences) {
                    ++res;
                }
            }
        }
        return res;
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
        bool changes;
        do {
            changes = false;
            for (int t = 0; t < lts.size(); t++) {
                for (int s = 0; s < lts.size(); s++) {
                    if (!lts.is_goal(s) || lts.is_goal(t)) { // TODO: Can avoid s == t
                        changes |= update_pair(s, t, label_relation);
                    }
                }
            }
            any_changes |= changes;
        } while (changes);
        return any_changes;
    }

    // Update the simulation relation for when t simulates s
    bool QualifiedLocalStateRelation2::update_pair(int s, int t, const QualifiedLabelRelation& label_relation) {
        // Remove all t-transitions for which it is not the case the label group of t-transition simulates the label group of the s-transition w.r.t. label_relation.
        // L simulates L' if for all l' in L', there exists l in L s.t. l' <=_i l, with <= being the label_relation for factor i
        bool changes = false;

        const auto& s_transitions = lts.get_transitions(s);
        const auto& t_transitions = lts.get_transitions(t);
        for (size_t s_tr_i = 0; s_tr_i < transition_responses[s][t].size(); ++s_tr_i) {
            const auto& s_tr = s_transitions[s_tr_i];
            auto& t_transitions_indices = transition_responses[s][t][s_tr_i];
            auto before = t_transitions_indices.size();

            changes |= 0 < std::erase_if(t_transitions_indices,
                [&](const int t_tr_i) {
                    if (t_tr_i == NOOP_TRANSITION) {
                        return !label_relation.label_group_simulated_by_noop(factor, s_tr.label_group);
                    } else {
                        const auto& t_tr = t_transitions[t_tr_i];
                        return !label_relation.label_group_simulates(factor, t_tr.label_group, s_tr.label_group);
                    }
            });
        }

        if (changes && simulations[t].contains(s)) {
            // Check if t still simulates s, i.e. for each of the s-transitions, there must be a t response s.t. the target
            // dominates the target of the s-transition
            for (size_t s_tr_i = 0; s_tr_i < transition_responses[s][t].size(); ++s_tr_i) {
                if (!lts.is_relevant_label_group(s_transitions.at(s_tr_i).label_group))
                    continue;
                if (!std::ranges::any_of(transition_responses[s][t][s_tr_i], [&](int t_tr_i) {
                    if (t_tr_i == NOOP_TRANSITION) {
                        return simulations.at(t).contains(s_transitions.at(s_tr_i).target);
                    }
                    return simulations.at(t_transitions.at(t_tr_i).target).contains(s_transitions.at(s_tr_i).target);
                })) {
                    simulations[t].erase(s);
                    break;
                }
            }
        }

        return changes;
    }

    bool QualifiedLocalStateRelation2::relation_initialize(int s, int t, const QualifiedLabelRelation& label_relation) {
        // Add to the s, t relation the t-transitions that simulate each s-transition, i.e. c(t_t) <= c(t_s)
        const auto& s_transitions = lts.get_transitions(s); // s -L> s'
        const auto& t_transitions = lts.get_transitions(t); // t -L'> t'
        for (size_t s_tr_i = 0; s_tr_i < s_transitions.size(); ++s_tr_i) {
            // Ignore irrelevant labels. Any state can simulate any irrelevant label with the same label, so we don't care about them
            if (!lts.is_relevant_label_group(s_transitions.at(s_tr_i).label_group)) {
                // std::println("Skip {}", lts.label_group_name(s_transitions.at(s_tr_i).label_group));
                continue;
            }
            for (size_t t_tr_i = 0; t_tr_i < t_transitions.size(); ++t_tr_i) {
                // Check that the t-transition simulates the s-transition, and that the target satisfies the goal condition
                if (label_relation.label_group_simulates(factor, t_transitions[t_tr_i].label_group, s_transitions[s_tr_i].label_group)
                    && (!lts.is_goal(s_transitions.at(s_tr_i).target) || lts.is_goal(t_transitions.at(t_tr_i).target)))  {
                    transition_responses[s][t][s_tr_i].emplace_back(t_tr_i);
                }
            }

            // Handle noop, i.e. t -noop-> t
            if (label_relation.label_group_simulated_by_noop(factor, s_transitions[s_tr_i].label_group)
                && (!lts.is_goal(s_transitions.at(s_tr_i).target) || lts.is_goal(t))) {
                transition_responses[s][t][s_tr_i].emplace_back(NOOP_TRANSITION);
            }
        }
        return true;
    }

    QualifiedLocalStateRelation2::QualifiedLocalStateRelation2(const LabelledTransitionSystem& lts, int factor, const fts::FTSTask& fts_task, const QualifiedLabelRelation& label_relation)
        : lts(lts), factor(factor), fts_task(fts_task) {

        // Initialize the relation. For each pair of states s_i, s_j and an s_i transition, the s_j transitions which dominate it
        // First, ensure that no non-goal state has a transition that dominates a goal states transition
        // Second, s -l> s' simulates t -l'> t' if c(l) <= c(l') (since we have no label relation yet)

        // Resize relation
        transition_responses.resize(lts.size());
        simulations.resize(lts.size());
        for (int s = 0; s < lts.size(); ++s) {
            transition_responses[s].resize(lts.size());
            for (int t = 0; t < lts.size(); ++t) {
                transition_responses[s][t].resize(lts.get_transitions(s).size());
                if (!lts.is_goal(s) || lts.is_goal(t)) {
                    // Only add t-transition responses if goal(s) => goal(t)
                    relation_initialize(s, t, label_relation);
                    simulations[t].insert(s);
                }
            }
        }
    }
}
