#include "state_dominance_relation.h"

#include "factor_dominance_relation.h"
#include "label_relation.h"
#include "../merge_and_shrink/transition_system.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../utils/logging.h"

using namespace std;

using merge_and_shrink::TransitionSystem;

namespace dominance {

    double StateDominanceRelation::get_percentage_simulations(bool ignore_equivalences) const {
        double percentage = 1;
        for (auto &sim: local_relations) {
            percentage *= static_cast<double>(sim->num_simulations()) / (sim->get_num_states() * sim->get_num_states());
        }
        if (ignore_equivalences) {
            percentage -= get_percentage_equivalences();
        } else {
            percentage -= get_percentage_equal();
        }
        return percentage;
    }

    double StateDominanceRelation::get_percentage_equal() const {
        double percentage = 1;
        for (auto &sim: local_relations) {
            percentage *= 1. / (sim->get_num_states() * sim->get_num_states());
        }
        return percentage;
    }

    double StateDominanceRelation::get_percentage_equivalences() const {
        double percentage = 1;
        for (auto &sim: local_relations) {
            int num_eq = 0;
            int num_states = sim->get_num_states();
            for (int i = 0; i < num_states; ++i)
                for (int j = 0; j < num_states; ++j)
                    if (sim->similar(i, j))
                        num_eq++;
            percentage *= num_eq / (static_cast<double>(num_states) * num_states);
        }
        return percentage;
    }


    int StateDominanceRelation::num_equivalences() const {
        int res = 0;
        for (size_t i = 0; i < local_relations.size(); i++) {
            res += local_relations[i]->num_equivalences();
        }
        return res;
    }

    int StateDominanceRelation::num_simulations() const {
        int res = 0;
        for (size_t i = 0; i < local_relations.size(); i++) {
            res += local_relations[i]->num_simulations() - local_relations[i]->get_num_states();
        }
        return res;
    }

    double StateDominanceRelation::num_st_pairs() const {
        double res = 1;
        for (size_t i = 0; i < local_relations.size(); i++) {
            res *= local_relations[i]->num_simulations() - local_relations[i]->get_num_states();
        }
        return res;
    }


    double StateDominanceRelation::num_states_problem() const {
        double res = 1;
        for (size_t i = 0; i < local_relations.size(); i++) {
            res *= local_relations[i]->get_num_states();
        }
        return res;
    }


    StateDominanceRelation::StateDominanceRelation(
        std::vector<std::unique_ptr<FactorDominanceRelation>>&& _local_relations,
        std::unique_ptr<LabelRelation>& label_relation):
        local_relations (std::move(_local_relations)), label_relation(std::move(label_relation)) {
    }

    void StateDominanceRelation::dump_statistics(utils::LogProxy &log) const {
        int num_equi = num_equivalences();
        int num_sims = num_simulations();

        int num_vars = 0;
        int num_vars_with_simulations = 0;
        for (size_t i = 0; i < local_relations.size(); i++) {
            if (!local_relations[i]->is_identity()) {
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
    }

}