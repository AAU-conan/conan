#include "local_state_relation.h"

#include "label_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../merge_and_shrink/transition_system.h"
#include "../utils/logging.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <ext/slist>

using namespace std;
using merge_and_shrink::TransitionSystem;
using fts::LabelledTransitionSystem;

namespace dominance {

    unique_ptr<LocalStateRelation> LocalStateRelation::get_local_distances_relation(const LabelledTransitionSystem & ts) {
        std::vector<std::vector<bool> > relation;
        int num_states = ts.size();
        const std::vector<bool> &goal_states = ts.get_goal_states();
        const std::vector<int> &goal_distances = ts.get_goal_distances();
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

        return make_unique<LocalStateRelation> (std::move(relation));
    }

    /*
    LocalStateRelation LocalStateRelation::get_local_distances_relation(const TransitionSystem & ts) {
        std::vector<std::vector<bool> > relation;
        int num_states = ts.get_size();
        const std::vector<bool> &goal_states = ts.get_goal_states();

        assert (ts.are_distances_computed());
        const std::vector<int> &goal_distances = ts.get_goal_distances();
        relation.resize(num_states);
        for (int i = 0; i < num_states; i++) {
            relation[i].resize(num_states, true);
            if (!goal_states[i]) {
                for (int j = 0; j < num_states; j++) {
                    if (goal_states[j] || goal_distances[i] > goal_distances[j]) {
                        relation[i][j] = false;
                    }
                }
            }
        }

        return LocalStateRelation {std::move(relation)};
    }
*//*

    LocalStateRelation LocalStateRelation::get_identity_relation(const TransitionSystem & ts) {
        std::vector<std::vector<bool> > relation;
        int num_states = ts.get_size();
        relation.resize(num_states);
        for (size_t i = 0; i < relation.size(); i++) {
            relation[i].resize(num_states);
            for (size_t j = 0; j < relation[i].size(); j++) {
                relation[i][j] = (i == j);
            }
        }
        return LocalStateRelation {std::move(relation)};
    }*/

    void LocalStateRelation::dump(utils::LogProxy &log, const vector <string> &names) const {
        log << "SIMREL:" << endl;
        for (size_t j = 0; j < relation.size(); ++j) {
            for (size_t i = 0; i < relation.size(); ++i) {
                if (simulates(j, i) && i != j) {
                    if (simulates(i, j)) {
                        if (j < i) {
                            log << names[i] << " <=> " << names[j] << endl;
                        }
                    } else {
                        log << names[i] << " <= " << names[j] << endl;
                    }
                }
            }
        }
    }

    void LocalStateRelation::dump(utils::LogProxy &log, const LabelledTransitionSystem& lts) const {
        log << "SIMREL:" << endl;
        for (size_t j = 0; j < relation.size(); ++j) {
            for (size_t i = 0; i < relation.size(); ++i) {
                if (simulates(j, i) && i != j) {
                    if (simulates(i, j)) {
                        if (j < i) {
                            log << lts.state_name(i) << " <=> " << lts.state_name(j) << endl;
                        }
                    } else {
                        log << lts.state_name(i) << " <= " << lts.state_name(j) << endl;
                    }
                }
            }
        }
    }

    void LocalStateRelation::dump(utils::LogProxy &log) const {
        log << "SIMREL:" << endl;
        for (size_t j = 0; j < relation.size(); ++j) {
            for (size_t i = 0; i < relation.size(); ++i) {
                if (simulates(j, i) && i != j) {
                    if (simulates(i, j)) {
                        if (j < i) {
                            log << i << " <=> " << j << endl;
                        }
                    } else {
                        log << i << " <= " << j << endl;
                    }
                }
            }
        }
    }


    int LocalStateRelation::num_equivalences() const {
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


    bool LocalStateRelation::is_identity() const {
        for (size_t i = 0; i < relation.size(); ++i) {
            for (size_t j = i + 1; j < relation.size(); ++j) {
                if (simulates(i, j) || simulates(j, i)) {
                    return false;
                }
            }
        }
        return true;
    }


    int LocalStateRelation::num_simulations(bool ignore_equivalences) const {
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

    int LocalStateRelation::num_different_states() const {
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
    double LocalStateRelation::get_percentage_simulations(bool ignore_equivalences) const {
        double num_sims = num_simulations(ignore_equivalences);
        double num_states = (ignore_equivalences ? num_different_states() : relation.size());
        return num_sims / (num_states * num_states);
    }

//Computes the probability of selecting a random pair s, s' such that
//s is equivalent to s'.
    double LocalStateRelation::get_percentage_equivalences() const {
        double num_eq = 0;
        double num_states = relation.size();
        for (size_t i = 0; i < relation.size(); ++i)
            for (size_t j = 0; j < relation.size(); ++j)
                if (similar(i, j))
                    num_eq++;
        return num_eq / (num_states * num_states);
    }

    void LocalStateRelation::compute_list_dominated_states() {
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

    void LocalStateRelation::cancel_simulation_computation() {
        vector<vector<bool> >().swap(relation);
    }

    LocalStateRelation::LocalStateRelation(vector <std::vector<bool>> &&relation) :
        relation(std::move(relation)){

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
