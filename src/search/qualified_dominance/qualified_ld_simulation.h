#ifndef DOMINANCE_LD_SIMULATION_H
#define DOMINANCE_LD_SIMULATION_H

#include <vector>
#include <ostream>
#include "qualified_factored_dominance_relation.h"
#include "qualified_local_state_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../utils/logging.h"
#include "../dominance/all_none_factor_index.h"

#include "../utils/timer.h"
#include "qualified_dominance_analysis.h"

#include <spot/tl/nenoform.hh>
#include <spot/tl/simplify.hh>

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
                    log << "Updating local relation " << i << std::endl;
                    update_local_relation(i, task.get_factor(i), label_relation, *(local_relations[i]), log);
                }
                log << " " << t() << std::flush;
            } while (label_relation.update(task, local_relations));
            log << std::endl << "LDSimulation finished: " << t() << std::endl;

            spot::tl_simplifier simp;

            for (auto [lts_id, lts] : std::views::enumerate(task.get_factors())) {
                const auto& relation = local_relations[lts_id];
                for (size_t j = 0; j < relation->num_states(); ++j) {
                    for (size_t i = 0; i < relation->num_states(); ++i) {
                        if (i != j) {
                            auto phi = relation->simulates_under(i, j);
                            if (phi == QualifiedFormula::tt()) {
                                log << lts->state_name(i) << " dominates " << lts->state_name(j) << std::endl;
                            } else if (phi != QualifiedFormula::ff()) {
                                log << lts->state_name(i) << " dominates " << lts->state_name(j) << " under  " << simp.simplify(phi) << std::endl;
                            }
                        }
                    }
                }
            }

        return std::make_unique<QualifiedFactoredDominanceRelation>(std::move(local_relations));
    }

    template<typename LR, typename LTS>
    void
    update_local_relation(int lts_id, const LTS &lts, const LR &label_dominance, QualifiedLocalStateRelation &local_relation, utils::LogProxy & log) {
        using QF = QualifiedFormula;

        bool changes = true;
        while (changes) {
            changes = false;
            for (int s = 0; s < lts.size(); s++) {
                for (int t = 0; t < lts.size(); t++) { //for each pair of states t, s
                    if (s != t && !spot::are_equivalent(local_relation.simulates_under(t , s), QF::ff())) {
                        log << "Checking if " << lts.state_name(t) << " simulates " << lts.state_name(s) << std::endl;
                        //Update the actions for which t simulates s
                        //for each transition s--l->s':
                        // a) with noop t >= s' and l dominated by noop?
                        // b) exist t--l'-->t', t' >= s' and l dominated by l'?

                        // Conjunctive formulas for all the transitions of s and responses from t
                        std::vector<QualifiedFormula> t_simulates_s;
                        lts.applyPostSrc(s, [&](const auto &trs) {
                            for (int trs_label : lts.get_labels(trs.label_group)) {
                                // s --l-> s'

                                // Disjunctive formulas for t's responses to when s chooses s --l-> s'
                                std::vector<QualifiedFormula> t_simulates_s_trs_label;
                                log << "    " << lts.state_name(s) << " has transition to " << lts.state_name(trs.target) << " with label " << lts.label_name(trs_label) << std::endl;

                                // Not l case, simply requires s to never choose l
                                t_simulates_s_trs_label.push_back(QF::G(QF::Not(QF::ap(lts.label_name(trs_label)))));

                                // Noop case, s' <= t and noop dominates l in all other LTSs.
                                QualifiedFormula local_noop = local_relation.simulates_under(t, trs.target);
                                if (local_noop != QF::ff()) {
                                    t_simulates_s_trs_label.push_back(QF::And({QF::X(QF::G(local_noop)), label_dominance.dominated_by_noop(trs_label, lts_id)}));
                                    log << "        " << lts.state_name(trs.target) << " is noop dominated under " << t_simulates_s_trs_label.back() << std::endl;
                                }

                                lts.applyPostSrc(t, [&](const auto &trt) {
                                    for (int trt_label: lts.get_labels(trt.label_group)) {
                                        QualifiedFormula local_next = local_relation.simulates_under(trt.target, trs.target);
                                        if (local_next == QF::ff())
                                            continue;
                                        QF f = QF::And({QF::X(QF::G(local_next)), label_dominance.dominates(trt_label, trs_label, lts_id)});
                                        t_simulates_s_trs_label.push_back(f);
                                        log << "        " << lts.state_name(t) << " has transition to " << lts.state_name(trt.target) << " with label " << lts.label_name(trt_label) << "; dominates under " << t_simulates_s_trs_label.back() << std::endl;
                                    }
                                    return false;
                                });

                                // Disjunction of all the responses of t
                                t_simulates_s.push_back(QF::Or(t_simulates_s_trs_label));
                                log << "    simulates under " << t_simulates_s.back() << std::endl;
                            }
                            return false;
                        });
                        // Conjunction of all the transitions of s
                        changes |= local_relation.set_simulates_under(t, s, QF::And(t_simulates_s));
                        log << lts.state_name(t) << " simulates " << lts.state_name(s) << " under " << local_relation.simulates_under(t, s) << std::endl;
                    }
                }
            }
        }
    }
}

#endif
