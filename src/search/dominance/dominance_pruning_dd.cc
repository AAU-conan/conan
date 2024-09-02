#include "dominance_pruning_dd.h"

#include "../factored_transition_system/factored_state_mapping.h"

#include "../factored_transition_system/symbolic_state_mapping.h"
using namespace std;

namespace dominance {


    unique_ptr <LocalStateRelationBDD>
    LocalStateRelationBDD::precompute_dominating_bdds(const fts::SymbolicStateMapping & symbolic_mapping,
                                                      const dominance::LocalStateRelation &state_relation) {
        std::vector<BDD> dominance_bdds;
        dominance_bdds.reserve(state_relation.num_states());
        for (int i = 0; i < state_relation.num_states(); ++i) {
            assert(state_relation.simulates(i, i));
            BDD dominance = symbolic_mapping.get_bdd(i);
            for (int j = 0; j < state_relation.num_states(); ++j) { //TODO: Here we could iterate over the dominating states
                if (state_relation.simulates(i, j) && i != j) {
                    dominance += symbolic_mapping.get_bdd(j);
                }
            }
            dominance_bdds.push_back(dominance);
        }
        return std::make_unique<LocalStateRelationBDD>(std::move(dominance_bdds));
    }


    unique_ptr <LocalStateRelationBDD>
    LocalStateRelationBDD::precompute_dominated_bdds(const fts::SymbolicStateMapping & symbolic_mapping,
                                                      const dominance::LocalStateRelation &state_relation) {
        std::vector<BDD> dominance_bdds;
        dominance_bdds.reserve(state_relation.num_states());
        for (int i = 0; i < state_relation.num_states(); ++i) {
            assert(state_relation.simulates(i, i));
            BDD dominance = symbolic_mapping.get_bdd(i);
            for (int j = 0; j < state_relation.num_states(); ++j) {
                if (state_relation.simulates(j, i) && i != j) {
                    dominance += symbolic_mapping.get_bdd(j);
                }
            }
            dominance_bdds.push_back(dominance);
        }
        return std::make_unique<LocalStateRelationBDD>(std::move(dominance_bdds));
    }


    BDD DominancePruningDD::getDominanceBDD(const State &state) const {
        state.unpack();
        BDD res = vars->oneBDD();
        try {
            for (int i = local_bdd_representation.size() - 1; i >= 0; --i) {
                int value = state_mapping->get_value(state.get_unpacked_values(), i);
                res *= local_bdd_representation[i]->get_dominance_bdd(value);
            }
        } catch (symbolic::BDDError e) {
            return vars->zeroBDD();
        }
        return res;
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

    void DominancePruningDD::initialize(const std::shared_ptr<AbstractTask> &task) {
        vector<int> var_order;
        ldSimulation->getVariableOrdering(var_order);

        vars->init(var_order, mgrParams);
        if (remove_spurious_dominated_states) {
            mgr = unique_ptr<SymManager>(new SymManager(vars.get(), nullptr, mgrParams, cost_type));
            mgr->init();
        }
        mgrParams.print_options();

        utils::Timer t;

        for (auto &local_relation: dominance_relation->get_local_relations()) {
            if (insert_dominated) {
                local_bdd_representation.push_back(LocalStateRelationBDD::precompute_dominated_bdds(vars, local_relation);
            } else {
                local_bdd_representation.push_back(LocalStateRelationBDD::precompute_dominating_bdds(vars, local_relation);
            }
        }
        log << "Precomputed dominance BDDs: " << t() << endl;


        BDD DominancePruningDD::getBDDToInsert(const State &state) {
            if (insert_dominated) {
                BDD res = ldSimulation->get_dominance_relation().getSimulatedBDD(vars.get(), state);
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
                if (pruning_type == PruningType::Generation) {
                    res -= vars->getStateBDD(state); //Remove the state
                } else if (vars->numStates(res) == 1) {
                    //Small optimization: If we have a single state, not include it
                    return vars->zeroBDD();
                }
                return res;
            } else {
                return vars->getStateBDD(state);
            }
        }

        void DominancePruningDD::print_statistics() {
            if (mgr) {
                cout << "Dominance BDD nodes: " << mgr->totalNodes() << endl;
                cout << "Dominance BDD memory: " << mgr->totalMemory() << endl;
            }
        }


/*
 *             add_option<bool>("insert_dominated",
                             "Whether we store the set of dominated states (default) or just the set of closed.",
                             "true");

    parser.add_enum_option
    ("pruning_dd", PruningDDValues,
    "Implementation data structure of the simulation pruning: "
    "BDD_MAP: (default) inserts in a map of BDDs all the dominated states "
    "ADD: inserts in an ADD all the dominated states "
    "BDD: inserts in a BDD all the dominated states (does"
    "not consider the g-value so it is not safe to use it with A* search)"
    "BDD_MAP_DISJ: allows a disjunctive factoritioning of BDDs in BDD_MAP "
    "SKYLINE_BDD_MAP, SKYLINE_BDD: inserts the real states instead of dominated states"
    "BDD_MAP");
*/


        std::ostream &operator<<(std::ostream &os, const PruningDD &pt) {
            switch (pt) {
                case PruningDD::BDD_MAP:
                    return os << "BDD map";
                case PruningDD::ADD:
                    return os << "ADD";
                case PruningDD::BDD:
                    return os << "BDD";
                case PruningDD::BDD_MAP_DISJ:
                    return os << "BDDmapDisj";
                case PruningDD::SKYLINE_BDD_MAP:
                    return os << "SkylineBDDmap";
                case PruningDD::SKYLINE_BDD:
                    return os << "SkylineBDD";
                default:
                    std::cerr << "Name of PruningTypeStrategy not known";
                    exit(-1);
            }
        }

        void DominancePruningBDDMap::insert(const State &state, int g) {
            BDD res = getBDDToInsert(state);
            if (!closed.count(g)) {
                closed[g] = res;
            } else {
                closed[g] += res;
            }
        }

        bool DominancePruningBDDMap::check(const State &state, int g) {
            if (insert_dominated) {
                auto sb = vars->getBinaryDescription(state);
                for (auto &entry: closed) {
                    if (entry.first > g) break;
                    if (!(entry.second.Eval(sb).IsZero())) {
                        return true;
                    }
                }
            } else {
                BDD simulatingBDD = ldSimulation->get_dominance_relation().getSimulatingBDD(vars.get(), state);
                for (auto &entry: closed) {
                    if (entry.first > g) break;
                    if (!((entry.second * simulatingBDD).IsZero())) {
                        return true;
                    }
                }
            }

            return false;
        }

        void DominancePruningBDD::insert(const State &state, int /*g*/) {
            if (!initialized) {
                closed = vars->zeroBDD();
                closed_inserted = vars->zeroBDD();
                initialized = true;
            }
            closed += getBDDToInsert(state);
        }

        bool DominancePruningBDD::check(const State &state, int /*g*/) {
            if (!initialized) {
                closed = vars->zeroBDD();
                closed_inserted = vars->zeroBDD();
                initialized = true;
            }

            if (insert_dominated) {
                auto sb = vars->getBinaryDescription(state);
                return !(closed.Eval(sb).IsZero());
            } else {
                BDD simulatingBDD = ldSimulation->get_dominance_relation().getSimulatingBDD(vars.get(), state);
                return !((closed * simulatingBDD).IsZero());
            }
        }

        void DominancePruningBDDMapDisj::insert(const State &state, int g) {
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

        bool DominancePruningBDDMapDisj::check(const State &state, int g) {
            utils::Timer t;

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
                BDD simulatingBDD = ldSimulation->get_dominance_relation().getSimulatingBDD(vars.get(), state);
                for (auto &entry: closed) {
                    if (entry.first > g) break;
                    for (auto &bdd: entry.second) {
                        if (!((bdd * simulatingBDD).IsZero())) {
                            return true;
                        }
                    }
                }
            }
            time_check += t();

            return false;
        }


        bool DominancePruningSkylineBDDMap::check(const State &state, int g) {
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

        void DominancePruningSkylineBDDMap::insert(const State &state, int g) {
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


        bool DominancePruningSkylineBDD::check(const State &state, int /*g*/) {
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
                    int val = local_relations[factor].get_index(state);
                    res *= closed[factor][val];
                } else {
                    BDD dom = vars->zeroBDD();
                    for (int valp: local_relations[factor].get_dominating_states(state)) {
                        dom += closed[factor][valp];
                    }
                    res *= dom;
                }
            }

            return !res.IsZero();
        }

        void DominancePruningSkylineBDD::insert(const State &state, int /*g*/) {
            auto &local_relations = dominance_relation->get_local_relations();

            if (closed.empty()) {
                closed.resize(local_relations.size());
                for (int factor = 0; factor < local_relations.size(); factor++) {
                    closed[factor].resize(local_relations[factor].num_states(), vars->zeroBDD());
                }
            }

            BDD bdd = vars->getStateBDD(state);
            for (int factor = 0; factor < local_relations.size(); factor++) {
                if (insert_dominated) {
                    for (int valp: local_relations[factor].get_dominated_states(state)) {
                        closed[factor][valp] += bdd;
                    }
                } else {
                    int val = local_relations[factor].get_index(state);
                    closed[factor][val] += bdd;
                }
            }
        }


    }

}