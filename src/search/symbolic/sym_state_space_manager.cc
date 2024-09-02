#include "sym_state_space_manager.h"

#include "../abstract_task.h"
#include "../plugins/options.h"
#include "../plugins/plugin.h"
#include "transition_relation.h"
#include <queue>

#include "bucket.h"

using namespace std;

namespace symbolic {
    SymStateSpaceManager::SymStateSpaceManager(SymVariables *v,
                                               const SymParamsMgr &params,
                                               const set<int> &relevant_vars_) :
            vars(v), p(params), relevant_vars(relevant_vars_),
            initialState(v->zeroBDD()), goal(v->zeroBDD()),
            min_transition_cost(0), hasTR0(false) {

        if (relevant_vars.empty()) {
            for (int i = 0; i < v->getTask().get_num_variables(); ++i) {
                relevant_vars.insert(i);
            }
        }
    }

/*
void SymStateSpaceManager::mergeBucket(Bucket &bucket) const {
    mergeOrBucket(bucket, p.max_aux_time, p.max_aux_nodes);
}

void SymStateSpaceManager::mergeBucketAnd(Bucket &bucket) const {
    mergeAndBucket(bucket, p.max_aux_time, p.max_aux_nodes);
}

void SymStateSpaceManager::shrinkBucket(Bucket &bucket, int maxNodes) {
    for (size_t i = 0; i < bucket.size(); ++i) {
        bucket[i] = shrinkExists(bucket[i], maxNodes);
    }
}*/

    void SymStateSpaceManager::init_transitions(const map<int, vector<std::shared_ptr<TransitionRelation>>> &(indTRs)) {
        transitions = indTRs; //Copy
        if (transitions.empty()) {
            hasTR0 = false;
            min_transition_cost = 1;
            return;
        }

        for (auto &entry: transitions) {
            merge(vars->get_bdd_manager(), entry.second, merge_uniqueTR, p.max_tr_time, p.max_tr_size);
        }

        min_transition_cost = transitions.begin()->first;
        if (min_transition_cost == 0) {
            hasTR0 = true;
            if (transitions.size() > 1) {
                min_transition_cost = (transitions.begin()++)->first;
            }
        }
    }

    SymParamsMgr::SymParamsMgr(const plugins::Options &opts) :
            max_tr_time(opts.get<double>("max_tr_time")),
            max_tr_size(opts.get<int>("max_tr_size")),
            max_aux_time(opts.get<double>("max_aux_time")),
            max_aux_nodes(opts.get<int>("max_aux_nodes")) {
        //Don't use edeletion with conditional effects

        // edeletion commented out for now
//    if (mutex_type == MutexType::MUTEX_EDELETION && has_conditional_effects()) {
//        cout << "Mutex type changed to mutex_and because the domain has conditional effects" << endl;
//        mutex_type = MutexType::MUTEX_AND;
//    }
    }

/*
SymParamsMgr::SymParamsMgr() :
    max_tr_size(100000), max_tr_time(60000), max_aux_nodes(1000000), max_aux_time(2000) {
    //Don't use edeletion with conditional effects

    // edeletion commented out for now
//    if (mutex_type == MutexType::MUTEX_EDELETION && has_conditional_effects()) {
//        cout << "Mutex type changed to mutex_and because the domain has conditional effects" << endl;
//        mutex_type = MutexType::MUTEX_AND;
//    }
}
*/

    void SymParamsMgr::print_options() const {
        if (utils::g_log.is_at_least_verbose()) {
            utils::g_log << "TR(time=" << max_tr_time << ", nodes=" << max_tr_size << ")" << endl;
            utils::g_log << "Aux(time=" << max_aux_time << ", nodes=" << max_aux_nodes << ")" << endl;
        }
    }

    void SymParamsMgr::add_options_to_feature(plugins::Feature &feature) {
        feature.add_option<int>("max_tr_size", "maximum size of TR BDDs", "100000");
        feature.add_option<double>("max_tr_time", "maximum time (ms) to generate TR BDDs", "60000");
        feature.add_option<int>("max_aux_nodes", "maximum size in pop operations", "1000000");
        feature.add_option<double>("max_aux_time", "maximum time (ms) in pop operations", "2000");

/*
    parser.add_enum_option("mutex_type", MutexTypeValues,
                           "mutex type", "MUTEX_EDELETION");

    parser.add_option<int> ("max_mutex_size",
                            "maximum size of mutex BDDs", "100000");

    parser.add_option<int> ("max_mutex_time",
                            "maximum time (ms) to generate mutex BDDs", "60000");
*/

    }

    std::ostream &operator<<(std::ostream &os, const SymStateSpaceManager &abs) {
        abs.print(os, false);
        return os;
    }

/*
    bool SymStateSpaceManager::isOriginal() const {
        return relevant_vars.size() == size_t(vars->getTask().get_num_variables());
    }
*/

    const map<int, std::vector<std::shared_ptr<TransitionRelation>>> &SymStateSpaceManager::getTransitions() const {
        return transitions;
    }

/*
    BDD SymStateSpaceManager::shrinkForall(const BDD &bdd) {
        setTimeLimit(p.max_aux_time);
        try{
            BDD res = shrinkForall(bdd, p.max_aux_nodes);
            unsetTimeLimit();
            return res;
        }catch (BDDError e) {
            unsetTimeLimit();
        }
        return zeroBDD();
    }
*/
}


