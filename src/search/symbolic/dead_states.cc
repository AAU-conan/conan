#include "dead_states.h"

#include "sym_variables.h"
#include "../abstract_task.h"

using namespace std;

namespace symbolic {

    DeadStates MutexManager::getMutexes(const SymVariables &vars) const {
        return DeadStates(mutex_type, vars);
    }

    DeadStates MutexManager::getMutexes(const SymVariables &vars) const {
        const auto &task = vars.getTask();

        //If (a) is initialized OR not using mutex OR edeletion does not need mutex
        if (mutex_type == MutexType::MUTEX_NOT)
            return DeadStates();     //Skip mutex initialization

        bool genMutexBDD = true;
        bool genMutexBDDByFluent = (mutex_type == MutexType::MUTEX_EDELETION);

        if (genMutexBDDByFluent) {
            //Initialize structure for exactlyOneBDDsByFluent (common to both init_mutex calls)
            exactlyOneBDDsByFluent.resize(task.get_num_variables());
            for (size_t i = 0; i < task.get_num_variables(); ++i) {

                exactlyOneBDDsByFluent[i].resize(task.get_variable_domain_size(i));
                for (int j = 0; j < task.get_variable_domain_size(i); ++j) {
                    exactlyOneBDDsByFluent[i][j] = vars->oneBDD();
                }
            }
        }
        init_mutex(mutex_groups, genMutexBDD, genMutexBDDByFluent, false);
        init_mutex(mutex_groups, genMutexBDD, genMutexBDDByFluent, true);

    }

    void MutexManager::init_mutex(const std::vector<MutexGroup> &mutex_groups,
                                  bool genMutexBDD, bool genMutexBDDByFluent, bool fw) {
        DEBUG_MSG(cout << "Init mutex BDDs " << (fw ? "fw" : "bw") << ": "
                       << genMutexBDD << " " << genMutexBDDByFluent << endl;);

        vector<vector<BDD>> &notMutexBDDsByFluent =
                (fw ? notMutexBDDsByFluentFw : notMutexBDDsByFluentBw);

        vector<BDD> &notMutexBDDs =
                (fw ? notMutexBDDsFw : notMutexBDDsBw);

        //BDD validStates = vars->oneBDD();
        int num_mutex = 0;
        int num_invariants = 0;

        if (genMutexBDDByFluent) {
            //Initialize structure for notMutexBDDsByFluent
            notMutexBDDsByFluent.resize(g_variable_domain.size());
            for (size_t i = 0; i < g_variable_domain.size(); ++i) {
                notMutexBDDsByFluent[i].resize(g_variable_domain[i]);
                for (int j = 0; j < g_variable_domain[i]; ++j) {
                    notMutexBDDsByFluent[i][j] = oneBDD();
                }
            }
        }

        //Initialize mBDDByVar and invariant_bdds_by_fluent
        vector<BDD> mBDDByVar;
        mBDDByVar.reserve(g_variable_domain.size());
        vector<vector<BDD>> invariant_bdds_by_fluent(g_variable_domain.size());
        for (size_t i = 0; i < invariant_bdds_by_fluent.size(); i++) {
            mBDDByVar.push_back(oneBDD());
            invariant_bdds_by_fluent[i].resize(g_variable_domain[i]);
            for (size_t j = 0; j < invariant_bdds_by_fluent[i].size(); j++) {
                invariant_bdds_by_fluent[i][j] = oneBDD();
            }
        }

        for (auto &mg: mutex_groups) {
            if (mg.pruneFW() != fw)
                continue;
            const vector<FactPair> &invariant_group = mg.getFacts();
            DEBUG_MSG(cout << mg << endl;);
            if (mg.isExactlyOne()) {
                BDD bddInvariant = zeroBDD();
                int var = numeric_limits<int>::max();
                int val = 0;
                bool exactlyOneRelevant = true;

                for (auto &fluent: invariant_group) {
                    if (!isRelevantVar(fluent.var)) {
                        exactlyOneRelevant = true;
                        break;
                    }
                    bddInvariant += vars->preBDD(fluent.var, fluent.value);
                    if (fluent.var < var) {
                        var = fluent.var;
                        val = fluent.value;
                    }
                }

                if (exactlyOneRelevant) {
                    num_invariants++;
                    if (genMutexBDD) {
                        invariant_bdds_by_fluent[var][val] *= bddInvariant;
                    }
                    if (genMutexBDDByFluent) {
                        for (auto &fluent: invariant_group) {
                            exactlyOneBDDsByFluent[fluent.var][fluent.value] *= bddInvariant;
                        }
                    }
                }
            }


            for (size_t i = 0; i < invariant_group.size(); ++i) {
                int var1 = invariant_group[i].var;
                if (!isRelevantVar(var1))
                    continue;
                int val1 = invariant_group[i].value;
                BDD f1 = vars->preBDD(var1, val1);

                for (size_t j = i + 1; j < invariant_group.size(); ++j) {
                    int var2 = invariant_group[j].var;
                    if (!isRelevantVar(var2))
                        continue;
                    int val2 = invariant_group[j].value;
                    BDD f2 = vars->preBDD(var2, val2);
                    BDD mBDD = !(f1 * f2);
                    if (genMutexBDD) {
                        num_mutex++;
                        mBDDByVar[min(var1, var2)] *= mBDD;
                        if (mBDDByVar[min(var1, var2)].nodeCount() > p.max_mutex_size) {
                            notMutexBDDs.push_back(mBDDByVar[min(var1, var2)]);
                            mBDDByVar[min(var1, var2)] = vars->oneBDD();
                        }
                    }
                    if (genMutexBDDByFluent) {
                        notMutexBDDsByFluent[var1][val1] *= mBDD;
                        notMutexBDDsByFluent[var2][val2] *= mBDD;
                    }
                }
            }
        }

        if (genMutexBDD) {
            for (size_t var = 0; var < g_variable_domain.size(); ++var) {
                if (!mBDDByVar[var].IsOne()) {
                    notMutexBDDs.push_back(mBDDByVar[var]);
                }
                for (const BDD &bdd_inv: invariant_bdds_by_fluent[var]) {
                    if (!bdd_inv.IsOne()) {
                        notMutexBDDs.push_back(bdd_inv);
                    }
                }
            }

            DEBUG_MSG(dumpMutexBDDs(fw););
            merge(vars, notMutexBDDs, mergeAndBDD,
                  p.max_mutex_time, p.max_mutex_size);
            std::reverse(notMutexBDDs.begin(), notMutexBDDs.end());
            DEBUG_MSG(cout << "Mutex initialized " << (fw ? "fw" : "bw") << ". Total mutex added: " << num_mutex
                           << " Invariant groups: " << num_invariants << endl;);
            DEBUG_MSG(dumpMutexBDDs(fw););
        }
    }


//For each op, include relevant mutexes
    void TransitionRelation::edeletion(const std::vector<std::vector<BDD>> &notMutexBDDsByFluentFw,
                                       const std::vector<std::vector<BDD>> &notMutexBDDsByFluentBw,
                                       const std::vector<std::vector<BDD>> &exactlyOneBDDsByFluent) {
        assert(ops.size() == 1);
        assert(notMutexBDDsByFluentFw.size() == sV->getTask().get_num_variables());
        assert(notMutexBDDsByFluentBw.size() == sV->getTask().get_num_variables());
        assert(exactlyOneBDDsByFluent.size() == sV->getTask().get_num_variables());

        TaskProxy task(sV->getTask());
        //For each op, include relevant mutexes
        for (const OperatorID op_id: ops) {
            const auto &op = task.get_operators()[op_id];
            for (const auto &pp: op.get_effects()) {
                int pp_var = pp.get_fact().get_variable().get_id();

                auto pre =
                        std::find_if(std::begin(op.get_preconditions()),
                                     std::end(op.get_preconditions()),
                                     [&pp](const auto &cond) {
                                         return pp.get_fact().get_variable().get_id() == cond.get_variable().get_id();
                                     });

                //edeletion bw
                if (pre == std::end(op.get_preconditions())) {
                    //We have a post effect over this variable.
                    //That means that every previous value is possible
                    //for each value of the variable
                    for (int val = 0; val < pp.get_fact().get_variable().get_domain_size(); val++) {
                        tBDD *= notMutexBDDsByFluentBw[pp_var][val];
                    }
                } else {
                    //In regression, we are making true pp.pre
                    //So we must negate everything of these.
                    tBDD *= notMutexBDDsByFluentBw[pp_var][(*pre).get_value()];
                }
                //edeletion fw
                tBDD *= notMutexBDDsByFluentFw[pp_var][pp.get_fact().get_value()].SwapVariables(swapVarsS, swapVarsSp);

                //edeletion invariants
                tBDD *= exactlyOneBDDsByFluent[pp_var][pp.get_fact().get_value()];
            }
        }
    }


    BDD SymStateSpaceManager::filter_mutex(const BDD &bdd, bool fw,
                                           int nodeLimit, bool initialization) {
        BDD res = bdd;
        const vector<BDD> &notDeadEndBDDs = ((fw || isAbstracted()) ? notDeadEndFw : notDeadEndBw);
        for (const BDD &notDeadEnd: notDeadEndBDDs) {
            DEBUG_MSG(cout << "Filter: " << res.nodeCount() << " and dead end " << notDeadEnd.nodeCount() << flush;);
            assert(!(notDeadEnd.IsZero()));
            res = res.And(notDeadEnd, nodeLimit);
            DEBUG_MSG(cout << ": " << res.nodeCount() << endl;);
        }

        const vector<BDD> &notMutexBDDs = (fw ? notMutexBDDsFw : notMutexBDDsBw);


        switch (p.mutex_type) {
            case MutexType::MUTEX_NOT:
                break;
            case MutexType::MUTEX_EDELETION:
                if (initialization) {
                    for (const BDD &notMutexBDD: notMutexBDDs) {
                        DEBUG_MSG(cout << res.nodeCount() << " and " << notMutexBDD.nodeCount() << flush;);
                        res = res.And(notMutexBDD, nodeLimit);
                        DEBUG_MSG(cout << ": " << res.nodeCount() << endl;);
                    }
                }
                break;
            case MutexType::MUTEX_AND:
                for (const BDD &notMutexBDD: notMutexBDDs) {
                    DEBUG_MSG(cout << "Filter: " << res.nodeCount() << " and " << notMutexBDD.nodeCount() << flush;);
                    res = res.And(notMutexBDD, nodeLimit);
                    DEBUG_MSG(cout << ": " << res.nodeCount() << endl;);
                }
                break;
            case MutexType::MUTEX_RESTRICT:
                for (const BDD &notMutexBDD: notMutexBDDs)
                    res = res.Restrict(notMutexBDD);
                break;
            case MutexType::MUTEX_NPAND:
                for (const BDD &notMutexBDD: notMutexBDDs)
                    res = res.NPAnd(notMutexBDD);
                break;
            case MutexType::MUTEX_CONSTRAIN:
                for (const BDD &notMutexBDD: notMutexBDDs)
                    res = res.Constrain(notMutexBDD);
                break;
            case MutexType::MUTEX_LICOMP:
                for (const BDD &notMutexBDD: notMutexBDDs)
                    res = res.LICompaction(notMutexBDD);
                break;
        }
        return res;
    }


    void DeadStates::addDeadEndStates(bool fw, BDD bdd) {
        //There are several options here, we could follow with edeletion
        // and modify the TRs, so that the new spurious states are never
        //generated. However, the TRs are already merged and they may get
        //too large. Therefore we just keep this states in another vectors
        //and spurious states are always removed.
        if (fw || isAbstracted()) {
            if (isAbstracted()) {
                bdd = shrinkForall(bdd);
            }
            aliveFw.push_back(!bdd);
            //mergeBucketAnd(notDeadEndFw);
        } else {
            aliveBw.push_back(!bdd);
            //mergeBucketAnd(notDeadEndBw);
        }
    }


    void DeadStates::addDeadStates(const std::vector<BDD> &fw_dead_ends, const std::vector<BDD> &bw_dead_ends) {
        for (BDD bdd: fw_dead_ends) {
            bdd = shrinkForall(bdd);
            if (!(bdd.IsZero())) {
                aliveFw.push_back(!bdd);
            }
        }

        for (BDD bdd: bw_dead_ends) {
            bdd = shrinkForall(bdd);
            if (!(bdd.IsZero())) {
                alive.push_back(!bdd);
            }
        }
        mergeBucketAnd(notDeadEndFw);
    }


    void DeadStates::dumpMutexBDDs(bool fw) const {
        if (fw) {
            cout << "Mutex BDD FW Size(" << p.max_mutex_size << "):";
            for (const auto &bdd: notMutexBDDsFw) {
                cout << " " << bdd.nodeCount();
            }
            cout << endl;
        } else {
            cout << "Mutex BDD BW Size(" << p.max_mutex_size << "):";
            for (const auto &bdd: notMutexBDDsBw) {
                cout << " " << bdd.nodeCount();
            }
            cout << endl;
        }
    }


    int SymStateSpaceManager::filterMutexBucket(vector <BDD> &bucket, bool fw,
                                                bool initialization, int maxTime, int maxNodes) {
        int numFiltered = 0;
        setTimeLimit(maxTime);
        try {
            for (size_t i = 0; i < bucket.size(); ++i) {
                DEBUG_MSG(cout << "Filter spurious " << (fw ? "fw" : "bw") << ": " << *this
                               << " from: " << bucket[i].nodeCount() <<
                               " maxTime: " << maxTime << " and maxNodes: " << maxNodes;);

                bucket[i] = filter_mutex(bucket[i], fw, maxNodes, initialization);
                DEBUG_MSG(cout << " => " << bucket[i].nodeCount() << endl;);
                numFiltered++;
            }
        } catch (BDDError e) {
            DEBUG_MSG(cout << " truncated." << endl;);
        }
        unsetTimeLimit();

        return numFiltered;
    }

    void SymStateSpaceManager::filterMutex(Bucket &bucket, bool fw, bool initialization) {
        filterMutexBucket(bucket, fw, initialization,
                          p.max_aux_time, p.max_aux_nodes);
    }


    std::ostream &operator<<(std::ostream &os, const MutexType &m) {
        switch (m) {
            case MutexType::MUTEX_NOT:
                return os << "not";
            case MutexType::MUTEX_EDELETION:
                return os << "edeletion";
            case MutexType::MUTEX_AND:
                return os << "and";
            case MutexType::MUTEX_RESTRICT:
                return os << "restrict";
            case MutexType::MUTEX_NPAND:
                return os << "npand";
            case MutexType::MUTEX_CONSTRAIN:
                return os << "constrain";
            case MutexType::MUTEX_LICOMP:
                return os << "licompaction";
            default:
                std::cerr << "Name of MutexType not known";
                utils::exit_with(utils::ExitCode::UNSUPPORTED);
        }
    }


}