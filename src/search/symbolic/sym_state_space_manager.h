#ifndef SYMBOLIC_SYM_STATE_SPACE_MANAGER_H
#define SYMBOLIC_SYM_STATE_SPACE_MANAGER_H

#include "bucket.h"
#include "sym_variables.h"
#include "transition_relation.h"
#include "../operator_cost.h"
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <cassert>

namespace plugins {
    class Feature;

    class Options;
}

namespace symbolic {
    class SymVariables;

    class TransitionRelation;

/*
 * All the methods may throw exceptions in case the time or nodes are exceeded.
 *
 */
    class SymParamsMgr {
    public:
        //Parameters to generate the TRs
        utils::Duration max_tr_time;
        long max_tr_size;

        //Time and memory bounds for auxiliary operations
        utils::Duration max_aux_time;
        long max_aux_nodes;

        SymParamsMgr(const plugins::Options &opts);

        static void add_options_to_feature(plugins::Feature &feature);

        void print_options() const;
    };

    class SymStateSpaceManager {

    protected:
        SymVariables *vars;
        const SymParamsMgr p;

        //If the variable is fully/partially/not considered in the abstraction
        std::set<int> relevant_vars;

        BDD initialState; // initial state
        BDD goal; // bdd representing the true (i.e. not simplified) goal-state

        std::map<int, std::vector<std::shared_ptr<TransitionRelation>>> transitions; //TRs
        int min_transition_cost; //minimum cost of non-zero cost transitions
        bool hasTR0; //If there is transitions with cost 0

        //BDD representation of valid states (wrt mutex) for fw and bw search
        // std::vector<BDD> notMutexBDDsFw, notMutexBDDsBw;

        //Dead ends for fw and bw searches. They are always removed in
        //filter_mutex (it does not matter which mutex_type we are using).
        // std::vector<BDD> notDeadEndFw, notDeadEndBw;

        BDD getRelVarsCubePre() const {
            return vars->getCubePre(relevant_vars);
        }

        BDD getRelVarsCubeEff() const {
            return vars->getCubeEff(relevant_vars);
        }

        virtual std::string tag() const = 0;

        void init_transitions(const std::map<int, std::vector<std::shared_ptr<TransitionRelation>>> &(indTRs));

    public:
        SymStateSpaceManager(SymVariables *v, const SymParamsMgr &params,
                             const std::set<int> &relevant_vars_ = std::set<int>());

        virtual ~SymStateSpaceManager() = default;

        inline SymVariables *getVars() const {
            return vars;
        }

        inline const std::set<int> &get_relevant_variables() const {
            return relevant_vars;
        }

        inline bool isRelevantVar(int var) const {
            return relevant_vars.count(var) > 0;
        }

        double stateCount(const DisjunctiveBucket &bucket) const {
            return vars->numStates(bucket);
        }

        inline const BDD &getGoal() {
            return goal;
        }

        inline const BDD &getInitialState() {
            return initialState;
        }

        inline BDD getBDD(int variable, int value) const {
            return vars->preBDD(variable, value);
        }

        long totalNodes() const {
            return vars->get_bdd_manager()->totalNodes();
        }

        unsigned long totalMemory() const {
            return vars->get_bdd_manager()->totalMemory();
        }

        inline BDD zeroBDD() const {
            return vars->zeroBDD();
        }

        inline BDD oneBDD() const {
            return vars->oneBDD();
        }

        //Methods that require of TRs initialized
        inline int getMinTransitionCost() const {
            assert(!transitions.empty());
            return min_transition_cost;
        }

        inline int getAbsoluteMinTransitionCost() const {
            assert(!transitions.empty());
            if (hasTR0) return 0;
            return min_transition_cost;
        }

        inline bool hasTransitions0() const {
            assert(!transitions.empty());
            return hasTR0;
        }

        const std::map<int, std::vector<std::shared_ptr<TransitionRelation>>> &getTransitions() const;

/*

    BDD filter_mutex(const BDD &bdd,
                     bool fw, int maxNodes,
                     bool initialization);

    int filterMutexBucket(std::vector<BDD> &bucket, bool fw,
                          bool initialization, int maxTime, int maxNodes);*/

        friend std::ostream &operator<<(std::ostream &os, const SymStateSpaceManager &state_space);

        virtual void print(std::ostream &os, bool /*fullInfo*/) const {
            os << tag() << " (" << relevant_vars.size() << ")";
        }

        //For plan solution reconstruction. Only available in original state space
        virtual const std::map<int, std::vector<std::shared_ptr<TransitionRelation>>> &getIndividualTRs() const {
            std::cerr << "Error: trying to get individual TRs from an invalid state space type" << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }

    };


}
#endif
