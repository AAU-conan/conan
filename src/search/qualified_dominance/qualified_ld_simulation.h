#ifndef DOMINANCE_LD_SIMULATION_H
#define DOMINANCE_LD_SIMULATION_H

#include <fstream>
#include <vector>
#include <ostream>
#include "qualified_factored_dominance_relation.h"
#include "qualified_local_state_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../utils/logging.h"
#include "../dominance/all_none_factor_index.h"

#include "qualified_dominance_analysis.h"
#include "../utils/timer.h"
#include "../utils/logging.h"


namespace fts {
    class FTSTask;
}

namespace qdominance {

    class QualifiedLDSimulation : public QualifiedDominanceAnalysis {
        utils::LogProxy log;
    public:
        QualifiedLDSimulation(utils::Verbosity verbosity);

        virtual ~QualifiedLDSimulation() = default;
        virtual std::unique_ptr<QualifiedFactoredDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) override;
    };



        template<typename LR>
        std::unique_ptr<QualifiedFactoredDominanceRelation> compute_ld_simulation(const fts::FTSTask & task, utils::LogProxy & log) {
            utils::Timer t;

            std::vector<std::unique_ptr<QualifiedLocalStateRelation>> local_relations;
            local_relations.reserve(task.get_num_variables());

            for (const auto & lts: task.get_factors()) {
                local_relations.push_back(QualifiedLocalStateRelation::get_local_distances_relation(*lts, task.get_num_labels()));
            }

            log << "Initialize qualified label dominance: " << task.get_num_labels() << " labels " << task.get_num_variables() << " systems." << std::endl;

            LR label_relation {task, local_relations};

            int total_size = 0, max_size = 0, total_trsize = 0, max_trsize = 0;
            for (const auto & lts: task.get_factors()) {
                max_size = std::max(max_size, lts->size());
                max_trsize = std::max(max_trsize, lts->num_transitions());
                total_size += lts->size();
                total_trsize += lts->num_transitions();
            }
            log << "Compute LDSim on " << task.get_num_variables() << " LTSs."
                      << " Total factor size: " << total_size
                      << ", total trsize: " << total_trsize
                      << ", max factor size: " << max_size
                      << ", max trsize: " << max_trsize
                      << std::endl;

            log << "Init LDSim in " << t() << ":" << std::endl << std::flush;
            do {
                for (size_t i = 0; i < local_relations.size(); i++) {
                    // log << "Updating local relation " << i << std::endl;
                    update_local_relation(i, task.get_factor(i), label_relation, *(local_relations[i]), log);
                }
                log << " " << t() << std::flush;
            } while (label_relation.update(task, local_relations));
            log << std::endl << "LDSimulation finished: " << t() << std::endl;

            for (const auto & [i, local_relation] : std::views::enumerate(local_relations)) {
#ifndef NDEBUG
                local_relation->draw_nfa(std::format("nfa_pre_reduce{}.dot", i));
#endif
                local_relation->reduce_nfa();
#ifndef NDEBUG
                local_relation->draw_transformed_nfa(std::format("nfa{}.dot", i), local_relation->get_nfa());
                std::cout << "state to nfa state map " << local_relation->get_state_to_nfa_state() << std::endl;
#endif
            }


            // for (auto [lts_id, lts] : std::views::enumerate(task.get_factors())) {
            //     const auto& relation = local_relations[lts_id];
            //     for (size_t j = 0; j < relation->num_states(); ++j) {
            //         for (size_t i = 0; i < relation->num_states(); ++i) {
            //             if (i != j) {
            //                 auto phi = relation->get_inverted_nfa(i, j);
            //                 relation->draw_transformed_nfa("inverted_nfa.dot", phi);
            //                 log << lts->state_name(i) << " simulates " << lts->state_name(j) << " except " << std::flush;
            //                 if (phi.is_universal(*phi.alphabet)) {
            //                     log << " everything " << std::endl;
            //                 } else {
            //                     for (auto w : phi.get_words(phi.num_of_states() - 1)) {
            //                         for (auto l : w) {
            //                             log << lts->label_name(l) << "->";
            //                         }
            //                         log << "; ";
            //                     }
            //                     log << std::endl;
            //                 }
            //             }
            //         }
            //     }
            // }

        return std::make_unique<QualifiedFactoredDominanceRelation>(std::move(local_relations));
    }

    template<typename LR, typename LTS>
    void
    update_local_relation(int lts_id, const LTS &lts, const LR &label_dominance, QualifiedLocalStateRelation &local_relation, utils::LogProxy & log) {
        bool changes = true;
        while (changes) {
            changes = false;
            for (int s = 0; s < lts.size(); s++) {
                for (int t = 0; t < lts.size(); t++) { //for each pair of states t, s
                    if (s != t && !local_relation.never_simulates(t, s)) {
                        // log << "Checking if " << lts.state_name(t) << " simulates " << lts.state_name(s) << std::endl;
                        //Update the actions for which t simulates s
                        //for each transition s--l->s':
                        // a) with noop t >= s' and l dominated by noop?
                        // b) exist t--l'-->t', t' >= s' and l dominated by l'?

                        changes |= local_relation.update(s, t, label_dominance, lts, lts_id, log);
                    }
                }
            }
        }
    }
}

#endif
