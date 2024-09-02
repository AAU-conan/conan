#ifndef DOMINANCE_LD_SIMULATION_H
#define DOMINANCE_LD_SIMULATION_H

#include <vector>
#include <ostream>
#include "factored_dominance_relation.h"
#include "local_state_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../utils/logging.h"

#include "../utils/timer.h"
#include "dominance_analysis.h"

namespace fts {
    class FTSTask;
}

namespace dominance {

    class LDSimulation : public DominanceAnalysis {
        utils::LogProxy log;
    public:
        LDSimulation(utils::Verbosity verbosity);

        virtual ~LDSimulation() = default;
        virtual std::unique_ptr<FactoredDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) override;
    };



        template<typename LR>
        std::unique_ptr<FactoredDominanceRelation> compute_ld_simulation(const fts::FTSTask & task, utils::LogProxy & log) {
        utils::Timer t;

        std::vector<std::unique_ptr<LocalStateRelation>> local_relations;
        local_relations.reserve(task.get_num_variables());
        for (const auto & lts: task.get_factors()) {
            local_relations.push_back(LocalStateRelation::get_local_distances_relation(*lts));
        }

        log << "Initialize label dominance: " << task.get_num_labels() << " labels " << task.get_num_variables() << " systems." << std::endl;

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

        log << "Init LDSim in " << t() << ":" << std::flush;
        do {
            for (size_t i = 0; i < local_relations.size(); i++) {
                update_local_relation(i, task.get_factor(i), label_relation, *(local_relations[i]));
            }
            log << " " << t() << std::flush;
        } while (label_relation.update(task, local_relations));
        log << std::endl << "LDSimulation finished: " << t() << std::endl;
/*
        if(dump) {
            for(int i = 0; i < _ltss.size(); i++){
                //_ltss[i]->dump();
                local_relations[i]->dump(_ltss[i]->get_names());
            }
        }
*/

        return std::make_unique<FactoredDominanceRelation>(std::move(local_relations));
    }

    template<typename LR, typename LTS>
    void
    update_local_relation(int lts_id, const LTS &lts, const LR &label_dominance, LocalStateRelation &local_relation) {
        bool changes = true;
        while (changes) {
            changes = false;
            for (int s = 0; s < lts.size(); s++) {
                for (int t = 0; t < lts.size(); t++) { //for each pair of states t, s
                    if (s != t && local_relation.simulates(t, s) /*&& !local_relation.fixed_simulates(t, s)*/) {
                        //log << "Checking states " << lts->name(s) << " and " << lts->name(t) << endl;
                        //Check if really t simulates s
                        //for each transition s--l->s':
                        // a) with noop t >= s' and l dominated by noop?
                        // b) exist t--l'-->t', t' >= s' and l dominated by l'?
                        lts.applyPostSrc(s, [&](const auto &trs) {
                            //log << "Checking transition " << s << " to " << trs.target << std::endl;

                            const std::vector<int> &labels_trs = lts.get_labels(trs.label_group);
                         //   assert(!labels_trs.empty());
                            for (size_t i = 0; i < labels_trs.size(); ++i) {
                                //log << "Checking label " << labels_trs[i] << " to " << trs.target << std::endl;
                                if (local_relation.simulates(t, trs.target) && label_dominance.dominated_by_noop(labels_trs[i], lts_id)) {
                                    continue;
                                }
                                bool found =
                                        lts.applyPostSrc(t, [&](const auto &trt) {
                                            if (local_relation.simulates(trt.target, trs.target)) {
                                                const std::vector<int> &labels_trt = lts.get_labels(trt.label_group);
                                                for (int label_trt: labels_trt) {
                                                    if (label_dominance.dominates(label_trt, labels_trs[i], lts_id)) {
                                                        return true;
                                                    }
                                                }
                                            }
                                            return false;
                                        });

                                if (!found) {
                                    changes = true;
                                    local_relation.remove(t, s);
                                    /*log << lts->name(t) << " does not simulate "
                                      << lts->name(s) << " because of "
                                      << lts->name(trs.src) << " => "
                                      << lts->name(trs.target) << " ("
                                      << trs.label << ")"; // << std::endl;
                                      log << "  Simulates? "
                                      << simulates(trs.src, trs.target);
                                      log << "  domnoop? "
                                      << label_dominance.dominated_by_noop(
                                      trs.label, lts_id) << "   ";
                                      label_dominance.dump(trs.label);*/
                                    /*for (auto trt : lts->get_transitions(t)) {
                                      log << "Tried with: "
                                      << lts->name(trt.src) << " => "
                                      << lts->name(trt.target) << " ("
                                      << trt.label << ")" << " label dom: "
                                      << label_dominance.dominates(trt.label,
                                      trs.label, lts_id)
                                      << " target sim "
                                      << simulates(trt.target, trs.target)
                                      << std::endl;
                                      }*/
                                    return true;
                                }
                            }

                            return false;
                        });
                    }
                }
            }
        }
    }
}

#endif
