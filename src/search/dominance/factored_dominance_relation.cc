#include "factored_dominance_relation.h"

#include "local_state_relation.h"
#include "../merge_and_shrink/transition_system.h"
#include "../merge_and_shrink/labels.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../utils/logging.h"

using namespace std;

using merge_and_shrink::TransitionSystem;

namespace dominance {

    double FactoredDominanceRelation::get_percentage_simulations(bool ignore_equivalences) const {
        double percentage = 1;
        for (auto &sim: local_relations) {
            percentage *= sim->get_percentage_simulations(false);
        }
        if (ignore_equivalences) {
            percentage -= get_percentage_equivalences();
        } else {
            percentage -= get_percentage_equal();
        }
        return percentage;
    }

    double FactoredDominanceRelation::get_percentage_equal() const {
        double percentage = 1;
        for (auto &sim: local_relations) {
            percentage *= 1 / (sim->num_states() * sim->num_states());
        }
        return percentage;
    }


    double FactoredDominanceRelation::get_percentage_equivalences() const {
        double percentage = 1;
        for (auto &sim: local_relations) {
            percentage *= sim->get_percentage_equivalences();
        }
        return percentage;
    }


    int FactoredDominanceRelation::num_equivalences() const {
        int res = 0;
        for (size_t i = 0; i < local_relations.size(); i++) {
            res += local_relations[i]->num_equivalences();
        }
        return res;
    }

    int FactoredDominanceRelation::num_simulations() const {
        int res = 0;
        for (size_t i = 0; i < local_relations.size(); i++) {
            res += local_relations[i]->num_simulations(true);
        }
        return res;
    }

    double FactoredDominanceRelation::num_st_pairs() const {
        double res = 1;
        for (size_t i = 0; i < local_relations.size(); i++) {
            res *= local_relations[i]->num_simulations(false);
        }
        return res;
    }


    double FactoredDominanceRelation::num_states_problem() const {
        double res = 1;
        for (size_t i = 0; i < local_relations.size(); i++) {
            res *= local_relations[i]->num_states();
        }
        return res;
    }


    void FactoredDominanceRelation::dump_statistics(utils::LogProxy &log) const {
        int num_equi = num_equivalences();
        int num_sims = num_simulations();

        int num_vars = 0;
        int num_vars_with_simulations = 0;
        for (size_t i = 0; i < local_relations.size(); i++) {
            if (local_relations[i]->num_simulations(true) > 0) {
                num_vars_with_simulations++;
            }
            num_vars++;
        }

        log << "Total Simulations: " << num_sims + num_equi * 2 << endl;
        log << "Similarity equivalences: " << num_equi << endl;
        log << "Only Simulations: " << num_sims << endl;
        log << "Simulations Found in " << num_vars_with_simulations << " out of " << num_vars << " variables" << endl;

        if (log.is_at_least_verbose()) {
            double num_pairs = num_st_pairs();
            double problem_size = num_states_problem();

            log << "Total st pairs: " << num_pairs << endl;
            log << "Percentage st pairs: " << num_pairs / (problem_size * problem_size) << endl;
        }
        /*for(int i = 0; i < local_relations.size(); i++){
          log << "States after simulation: " << local_relations[i]->num_states() << " "
          << local_relations[i]->num_different_states() << endl;
          }*/
    }
/*
    bool FactoredDominanceRelation::pruned_state(const State &state) const {
        for (auto &sim: local_relations) {
            if (sim->pruned(state)) {
                return true;
            }
        }
        return false;
    }

    int FactoredDominanceRelation::get_cost(const State &state) const {
        int cost = 0;
        for (auto &sim: local_relations) {
            int new_cost = sim->get_cost(state);
            if (new_cost == -1) return -1;
            cost = max(cost, new_cost);
        }
        return cost;
    }

    bool FactoredDominanceRelation::dominates(const State &t, const State &s) const {
        for (auto &sim: local_relations) {
            if (!sim->simulates(t, s)) {
                return false;
            }
        }
        return true;
    }*/

}