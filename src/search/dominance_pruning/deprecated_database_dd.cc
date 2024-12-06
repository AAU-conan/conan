#include "database_dd.h"

#include "../factored_transition_system/factored_state_mapping.h"
#include "../factored_transition_system/symbolic_state_mapping.h"
#include "../variable_ordering/variable_ordering_strategy.h"

using namespace std;
using namespace symbolic;

namespace dominance {
    DominancePruningDD::DominancePruningDD(bool insert_dominated, bool remove_spurious_dominated_states,
                                           const BDDManagerParameters &bdd_mgr_params,
                                           std::shared_ptr<variable_ordering::VariableOrderingStrategy>
                                           variable_ordering_strategy,
                                           const std::shared_ptr<fts::FTSTaskFactory> &fts_factory,
                                           std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                           utils::Verbosity verbosity) : DominancePruningPrevious(fts_factory,
                                                                             dominance_analysis,
                                                                             verbosity),
                                                                         bdd_mgr(make_shared<symbolic::BDDManager>(
                                                                             bdd_mgr_params)),
                                                                         variable_ordering_strategy(
                                                                             variable_ordering_strategy),
                                                                         insert_dominated(insert_dominated),
                                                                         remove_spurious_dominated_states(
                                                                             remove_spurious_dominated_states) {
    }


    DominanceRelationBDD::DominanceRelationBDD(const FactoredDominanceRelation &dominance_relation,
                                               const fts::FactoredSymbolicStateMapping &symbolic_mapping,
                                               bool dominated) : dominated(dominated) {
        assert(dominance_relation.size() == symbolic_mapping.size());

    }

    void DominancePruningDD::initialize(const std::shared_ptr<AbstractTask> &task) {
        DominancePruningPrevious::initialize(task);

        utils::Timer t;

        auto vars = make_shared<SymVariables>(bdd_mgr, *variable_ordering_strategy, task);

        fts::FactoredSymbolicStateMapping mapping (*state_mapping, vars);

        dominance_relation_bdd = std::make_unique<DominanceRelationBDD>(*dominance_relation, mapping);
        /* TODO: Add option of removing spurious states, needs symbolic representation of the mutex information
                if (remove_spurious_dominated_states) {
                    mgr = unique_ptr<SymManager>(new SymManager(vars.get(), nullptr, mgrParams, cost_type));
                    mgr->init();
                }
        */
        log << "Precomputed dominance BDDs: " << t() << endl;
    }



    /*
        BDD DominancePruningDD::getIrrelevantStates(SymVariables *vars) const {
            BDD res = vars->zeroBDD();
            try {
                for (auto it = simulations.rbegin(); it != simulations.rend(); it++) {
                    res += (*it)->getIrrelevantStates(vars);
                }
            } catch (BDDError e) {
                return vars->zeroBDD();
            }
            return res;
        }
    */

    //    BDD DominanceRelationBDD::setOfDominatingStates(const ExplicitState &state) const {
    //        try {
    //            BDD res = vars->oneBDD();
    //            for (int i = local_bdd_representation.size() - 1; i >= 0; --i) {
    //                int value = state[i];
    //                res *= local_bdd_representation[i]->get_dominance_bdd(value);
    //            }
    //            return res;
    //        } catch (symbolic::BDDError) {
    //            // This should never happen.
    //            return vars->getStateBDD(state);
    //        }
    //    }
    //



    BDD DominancePruningDD::getBDDToInsert(const ExplicitState &state) const {
        if (insert_dominated) {
            auto res = dominance_relation_bdd->setOfDominatedStates(state);

            /*
                        if (remove_spurious_dominated_states) {
                            try {
                                res = mgr->filter_mutex(res, true, 1000000, true);
                                res = mgr->filter_mutex(res, false, 1000000, true);
                            } catch (BDDError) {
                                cout << "Disabling removal of spurious dominated states" << endl;
                                //If it is not possible, do not remove spurious states
                                remove_spurious_dominated_states = false;
                            }
                        }
            */
            if (vars->numStates(res) == 1) {
                //Small optimization: If we have a single state, not include it
                return vars->zeroBDD();
            }

            return res;
        } else {
            return vars->getStateBDD(state);
        }
    }



    // DominancePruningBDD::DominancePruningBDD(bool insert_dominated, bool remove_spurious_dominated_states,
    // const symbolic::BDDManagerParameters &bdd_mgr_params,
    // std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy,
    // const std::shared_ptr<fts::FTSTaskFactory> &fts_factory, std::shared_ptr<DominanceAnalysis> dominance_analysis,
    // utils::Verbosity verbosity):
    // DominancePruningDD(insert_dominated, remove_spurious_dominated_states, bdd_mgr_params, variable_ordering_strategy, fts_factory, dominance_analysis,
    //                    verbosity) {
    //     //TODO: Error: vars has not been initialized in the constructor
    //     closed = vars->zeroBDD();
    //     closed_inserted = vars->zeroBDD();
    // }

    // void DominancePruningBDD::insert(const ExplicitState &state, int /*g*/) {
    //     closed += getBDDToInsert(state);
    // }
    //
    // bool DominancePruningBDD::check(const ExplicitState &state, int /*g*/) const {
    //     if (insert_dominated) {
    //         auto sb = vars->getBinaryDescription(state);
    //         return !(closed.Eval(sb).IsZero());
    //     } else {
    //         BDD simulatingBDD = dominance_relation_bdd->setOfDominatingStates(state);
    //         return !((closed * simulatingBDD).IsZero());
    //     }
    // }

    void DominancePruningBDDMapDisj::insert(const ExplicitState &state, int g) {
        BDD res = getBDDToInsert(state);

        if (!closed[g].empty() && closed[g].back().nodeCount() <= 1000) {
            closed[g][closed[g].size() - 1] += res;
        } else {
            closed[g].push_back(res);
        }

        /*
                    time_insert += t();
                    if (states_inserted % 1000 == 0) {
                        cout << "SimulationClosed: ";
                        for (auto &entry: closed) {
                            cout << " " << entry.first << "(" << entry.second.size() << "),";
                        }
                        cout << " after " << states_inserted << endl;
                        cout << time_bdd << ", " << time_insert << " and " << time_check << " of " << g_timer() << endl;
                    }
        */
    }


    bool DominancePruningBDDMapDisj::check(const ExplicitState &state, int g) const {
        if (insert_dominated) {
            auto sb = vars->getBinaryDescription(state);
            for (auto &entry: closed) {
                if (entry.first > g) break;
                for (auto &bdd: entry.second) {
                    if (!(bdd.Eval(sb).IsZero())) {
                        return true;
                    }
                }
            }
        } else {
            BDD simulatingBDD = dominance_relation_bdd->setOfDominatingStates(state);
            for (auto &entry: closed) {
                if (entry.first > g) break;
                for (auto &bdd: entry.second) {
                    if (!((bdd * simulatingBDD).IsZero())) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /*

        bool DominancePruningSkylineBDDMap::check(const ExplicitState &state, int g) const {
            auto &local_relations = dominance_relation->get_local_relations();

            if (closed.empty()) {
                closed.resize(local_relations.size());
                for (int factor = 0; factor < local_relations.size(); factor++) {
                    closed[factor].resize(local_relations[factor].num_states());
                }
            }

            for (int gval: g_values) {
                if (gval > g) {
                    return false;
                }

                BDD res = vars->oneBDD();

                for (int factor = 0; !res.IsZero() && factor < local_relations.size(); factor++) {
                    if (insert_dominated) {
                        int val = local_relations[factor].get_index(state);
                        if (closed[factor][val].count(gval)) {
                            res *= closed[factor][val][gval];
                        } else {
                            return false;
                        }
                    } else {
                        BDD dom = vars->zeroBDD();
                        for (int valp: local_relations[factor].get_dominating_states(state)) {
                            if (closed[factor][valp].count(gval)) {
                                dom += closed[factor][valp][gval];
                            }
                        }
                        res *= dom;
                    }
                }

                if (!res.IsZero()) {
                    return true;
                }
            }
            return false;
        }

        void DominancePruningSkylineBDDMap::insert(const ExplicitState &state, int g) {
            g_values.insert(g);
            const auto &local_relations = dominance_relation->get_local_relations();

            if (closed.empty()) {
                closed.resize(local_relations.size());
                for (int factor = 0; factor < local_relations.size(); factor++) {
                    closed[factor].resize(local_relations[factor].num_states());
                }
            }

            BDD bdd = vars->getStateBDD(state);

            for (int factor = 0; factor < local_relations.size(); factor++) {
                if (insert_dominated) {
                    for (int valp: local_relations[factor].get_dominated_states(state)) {
                        if (!closed[factor][valp].count(g)) {
                            closed[factor][valp][g] = bdd;
                        } else {
                            closed[factor][valp][g] += bdd;
                        }
                    }
                } else {
                    int val = local_relations[factor].get_index(state);
                    if (!closed[factor][val].count(g)) {
                        closed[factor][val][g] = bdd;
                    } else {
                        closed[factor][val][g] += bdd;
                    }
                }
            }
        }


        bool DominancePruningSkylineBDD::check(const ExplicitState &state, int */
    /*g*/ /*
) const {
        const auto &local_relations = dominance_relation->get_local_relations();

        if (closed.empty()) {
            closed.resize(local_relations.size());
            for (int factor = 0; factor < local_relations.size(); factor++) {
                closed[factor].resize(local_relations[factor].num_states(), vars->zeroBDD());
            }
        }

        BDD res = vars->oneBDD();

        for (int factor = 0; !res.IsZero() && factor < local_relations.size(); factor++) {
            if (insert_dominated) {
                res *= closed[factor][state[factor]];
            } else {
                BDD dom = vars->zeroBDD();
                for (int valp: local_relations[factor]->get_dominating_states(state[factor])) {
                    dom += closed[factor][valp];
                }
                res *= dom;
            }
        }

        return !res.IsZero();
    }

    void DominancePruningSkylineBDD::insert(const ExplicitState &state, int */
    /*g*/ /*
) {
        auto &local_relations = dominance_relation->get_local_relations();

        if (closed.empty()) {
            closed.resize(local_relations.size());
            for (int factor = 0; factor < local_relations.size(); factor++) {
                closed[factor].resize(local_relations[factor]->num_states(), vars->zeroBDD());
            }
        }

        BDD bdd = vars->getStateBDD(state);
        for (int factor = 0; factor < local_relations.size(); factor++) {
            if (insert_dominated) {
                for (int valp: local_relations[factor]->get_dominated_states(state[factor])) {
                    closed[factor][valp] += bdd;
                }
            } else {
                closed[factor][state[factor]] += bdd;
            }
        }
    }
}
*/

}
