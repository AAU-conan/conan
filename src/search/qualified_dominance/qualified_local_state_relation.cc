#include "qualified_local_state_relation.h"

#include "../dominance/label_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../merge_and_shrink/transition_system.h"
#include "../utils/logging.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <ext/slist>

using namespace std;
using merge_and_shrink::TransitionSystem;
using fts::LabelledTransitionSystem;

namespace qdominance {

    unique_ptr<QualifiedLocalStateRelation> QualifiedLocalStateRelation::get_local_distances_relation(const LabelledTransitionSystem & ts, int num_labels) {
        std::vector<std::vector<QualifiedFormula> > relation;

        int num_states = ts.size();
        const std::vector<bool> &goal_states = ts.get_goal_states();
        relation.resize(num_states);
        for (int i = 0; i < num_states; i++) {
            relation[i].resize(num_states, QualifiedFormula::tt());
            if (!goal_states[i]) {
                for (int j = 0; j < num_states; j++) {
                    if (goal_states[j]) {
                        // Set to false; no paths allow a non-goal state to dominate a goal state
                        relation[i][j] = QualifiedFormula::ff();
                    }
                }
            }
        }

        return make_unique<QualifiedLocalStateRelation> (std::move(relation), num_labels);
    }

    int QualifiedLocalStateRelation::num_equivalences() const {
        int num = 0;
        std::vector<bool> counted(relation.size(), false);
        for (size_t i = 0; i < counted.size(); i++) {
            if (!counted[i]) {
                for (size_t j = i + 1; j < relation.size(); j++) {
                    if (similar(i, j)) {
                        counted[j] = true;
                    }
                }
            } else {
                num++;
            }
        }
        return num;
    }

    int QualifiedLocalStateRelation::num_simulations(bool ignore_equivalences) const {
        int res = 0;
        if (ignore_equivalences) {
            std::vector<bool> counted(relation.size(), false);
            for (size_t i = 0; i < relation.size(); ++i) {
                if (!counted[i]) {
                    for (size_t j = i + 1; j < relation.size(); ++j) {
                        if (similar(i, j)) {
                            counted[j] = true;
                        }
                    }
                }
            }
            for (size_t i = 0; i < relation.size(); ++i) {
                if (!counted[i]) {
                    for (size_t j = i + 1; j < relation.size(); ++j) {
                        if (!counted[j]) {
                            if (!similar(i, j) && (simulates(i, j) || simulates(j, i))) {
                                res++;
                            }
                        }
                    }
                }
            }
        } else {
            for (size_t i = 0; i < relation.size(); ++i)
                for (size_t j = 0; j < relation.size(); ++j)
                    if (simulates(i, j))
                        res++;
        }
        return res;
    }

    int QualifiedLocalStateRelation::num_different_states() const {
        int num = 0;
        std::vector<bool> counted(relation.size(), false);
        for (size_t i = 0; i < counted.size(); i++) {
            if (!counted[i]) {
                num++;
                for (size_t j = i + 1; j < relation.size(); j++) {
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
    double QualifiedLocalStateRelation::get_percentage_simulations(bool ignore_equivalences) const {
        double num_sims = num_simulations(ignore_equivalences);
        double num_states = (ignore_equivalences ? num_different_states() : relation.size());
        return num_sims / (num_states * num_states);
    }

//Computes the probability of selecting a random pair s, s' such that
//s is equivalent to s'.
    double QualifiedLocalStateRelation::get_percentage_equivalences() const {
        double num_eq = 0;
        double num_states = relation.size();
        for (size_t i = 0; i < relation.size(); ++i)
            for (size_t j = 0; j < relation.size(); ++j)
                if (similar(i, j))
                    num_eq++;
        return num_eq / (num_states * num_states);
    }

    void QualifiedLocalStateRelation::compute_list_dominated_states() {
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

    void QualifiedLocalStateRelation::cancel_simulation_computation() {
        vector<vector<QualifiedFormula> >().swap(relation);
    }

    QualifiedLocalStateRelation::QualifiedLocalStateRelation(vector <std::vector<QualifiedFormula>> &&relation, int num_labels) :
        relation(std::move(relation)), num_labels(num_labels) {

    }

    /*
    bool LocalStateRelation::simulates(const State &t, const State &s) const {
        int tid = abs->get_abstract_state(t);
        int sid = abs->get_abstract_state(s);
        return relation[tid][sid];
    }


    bool LocalStateRelation::pruned(const State &state) const {
        return abs->get_abstract_state(state) == -1;
    }

    int LocalStateRelation::get_cost(const State &state) const {
        return abs->get_cost(state);
    }

    int LocalStateRelation::get_index(const State &state) const {
        return abs->get_abstract_state(state);
    }

    const std::vector<int> &LocalStateRelation::get_dominated_states(const State &state) {
        if (dominated_states.empty()) {
            compute_list_dominated_states();
        }
        return dominated_states[abs->get_abstract_state(state)];
    }

    const std::vector<int> &LocalStateRelation::get_dominating_states(const State &state) {
        if (dominated_states.empty()) {
            compute_list_dominated_states();
        }
        return dominating_states[abs->get_abstract_state(state)];
    }
*/

}
