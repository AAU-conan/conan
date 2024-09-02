#ifndef SYMBOLIC_SYM_MUTEXES_H
#define SYMBOLIC_SYM_MUTEXES_H

#include "bucket.h"
#include <vector>

namespace symbolic {
    class SymVariables;

    enum class MutexType {
        MUTEX_NOT, MUTEX_AND, MUTEX_RESTRICT, MUTEX_NPAND, MUTEX_CONSTRAIN, MUTEX_LICOMP, MUTEX_EDELETION
    };

    std::ostream &operator<<(std::ostream &os, const MutexType &m);

    extern const std::vector<std::string> MutexTypeValues;

    class DeadStates {
        MutexType mutex_type;


        //TODO: Conjunctive Bucket

        //BDD representation of valid states (wrt mutex) for fw and bw search
        std::vector<BDD> notMutexBDDsFw, notMutexBDDsBw;
        std::vector<BDD> alive;   // States that have been identified as alive (on top of mutexes)

        //notMutex relative for each fluent
        std::vector<std::vector<BDD>> notMutexBDDsByFluentFw, notMutexBDDsByFluentBw;
        std::vector<std::vector<BDD>> exactlyOneBDDsByFluent;

    public:
        DeadStates(MutexType m_type, const SymVariables &vars);

        /* std::vector<BDD> &&notMutexBDDsFw,
                   std::vector<BDD> &&notMutexBDDsBw,
                   std::vector<std::vector<BDD>> && notMutexBDDsByFluentFw,
                   std::vector<std::vector<BDD>> && notMutexBDDsByFluentBw,
                   std::vector<std::vector<BDD>> && exactlyOneBDDsByFluent) ;

        DeadStates(MutexType m_type, std::vector<BDD> &&notMutexBDDsFw,
                   std::vector<BDD> &&notMutexBDDsBw) ;
*/
        BDD filter_dead(const BDD &bdd, bool fw, int nodeLimit, bool initialization) const;
    };

    class MutexManager {
        //Parameters to generate the mutex BDDs
        MutexType mutex_type;
        int max_mutex_size, max_mutex_time;

    public:
        MutexManager(const plugins::Options &opts);

        virtual DeadStates getMutexes(const SymVariables &vars) const;

        static void add_options_to_feature(plugins::Feature &feature);
/*
        //Methods that require of mutex initialized
        inline const BDD &getNotMutexBDDFw(int var, int val) const {
            return notMutexBDDsByFluentFw[var][val];
        }

        //Methods that require of mutex initialized
        inline const BDD &getNotMutexBDDBw(int var, int val) const {
            return notMutexBDDsByFluentBw[var][val];
        }

        //Methods that require of mutex initialized
        inline const BDD &getExactlyOneBDD(int var, int val) const {
            return exactlyOneBDDsByFluent[var][val];
        }*/

    };
}


#endif