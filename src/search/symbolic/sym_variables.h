#ifndef SYMBOLIC_SYM_VARIABLES_H
#define SYMBOLIC_SYM_VARIABLES_H

#include "bdd_manager.h"
#include "bucket.h"

#include "../utils/timer.h"
#include <math.h>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <map>
#include <cassert>

class State;

class FactPair;

class AbstractTask;

namespace variable_ordering {
    class VariableOrderingStrategy;
}

namespace symbolic {
    typedef unsigned long long int StateCountType;


/*
 * BDD-Variables for a symbolic exploration.
 * This information is global for every class using symbolic search.
 * The only decision fixed here is the variable ordering, which is assumed to be always fixed.
 */
    class SymVariables {
        // Var order used by the algorithm.
        //const VariableOrderType variable_ordering;
        const std::shared_ptr<BDDManager> bdd_manager;
        const std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy;
        //const std::shared_ptr<AbstractTask> task;

        int numBDDVars = 0; //Number of binary variables (just one set, the total number is numBDDVars*3
        std::vector<BDD> variables; // BDD variables


        //The variable order must be complete.
        std::vector<int> var_order; //Variable(FD) order in the BDD
        std::vector<int> variable_domain_sizes; //Domain sizes of the variables
        std::vector<std::vector<int>> bdd_index_pre, bdd_index_eff; //vars(BDD) for each var(FD)

        std::vector<std::vector<BDD>> preconditionBDDs; // BDDs associated with the precondition of a predicate
        std::vector<std::vector<BDD>> effectBDDs;      // BDDs associated with the effect of a predicate
        std::vector<BDD> biimpBDDs;  //BDDs associated with the biimplication of one variable(FD)
        std::vector<BDD> validValues; // BDD that represents the valid values of all the variables
        BDD validBDD;  // BDD that represents the valid values of all the variables

        //Vector to store the binary description of an state
        //Avoid allocating memory during heuristic evaluation
        mutable std::vector<char> binStateChar;
        mutable std::vector<int> binState;


        void constructor(); //Assumes bdd_manager, var_order, and variable_domain_sizes are already initialized
    public:
        SymVariables(std::shared_ptr<BDDManager> manager, std::vector<int> var_order, std::vector<int> variable_domain_sizes);
        SymVariables(std::shared_ptr<BDDManager> manager, const variable_ordering::VariableOrderingStrategy &variable_ordering,
                     std::shared_ptr<AbstractTask> task);

        size_t get_num_variables () const {
            return preconditionBDDs.size();
        }

        size_t get_domain_size (int var) const {
            return variable_domain_sizes[var];
        }

        //State getStateFrom(const BDD & bdd) const;
        BDD getStateBDD(const State &state) const;

        BDD getStateBDD(const std::vector<int> &state) const;

        BDD getPartialStateBDD(const std::vector<std::pair<int, int>> &state) const;

        BDD getPartialStateBDD(const std::vector<FactPair> &state) const;

        StateCountType numStates(const BDD &bdd) const; //Returns the number of states in a BDD
        StateCountType numStates() const;

        StateCountType numStates(const DisjunctiveBucket &bucket) const;

        BDDManager *get_bdd_manager() const {
            return bdd_manager.get();
        }

        // const AbstractTask &getTask() const {
        //     return *task;
        // }

        double percentageNumStates(const BDD &bdd) const {
            return numStates(bdd) / numStates();
        }

        bool isIn(const State &state, const BDD &bdd) const;

        inline const std::vector<int> &vars_index_pre(int variable) const {
            return bdd_index_pre[variable];
        }

        inline const std::vector<int> &vars_index_eff(int variable) const {
            return bdd_index_eff[variable];
        }

        BDD oneBDD() const {
            return bdd_manager->oneBDD();
        }

        BDD zeroBDD() const {
            return bdd_manager->zeroBDD();
        }

        inline const BDD &preBDD(int variable, int value) const {
            return preconditionBDDs[variable][value];
        }

        inline const BDD &effBDD(int variable, int value) const {
            return effectBDDs[variable][value];
        }


        inline BDD getCubePre(int var) const {
            return getCube(var, bdd_index_pre);
        }

        inline BDD getCubePre(const std::set<int> &vars) const {
            return getCube(vars, bdd_index_pre);
        }

        inline BDD getCubeEff(int var) const {
            return getCube(var, bdd_index_eff);
        }

        inline BDD getCubeEff(const std::set<int> &vars) const {
            return getCube(vars, bdd_index_eff);
        }

        inline const BDD &biimp(int variable) const {
            return biimpBDDs[variable];
        }

        inline std::vector<BDD> getBDDVarsPre() const {
            return getBDDVars(var_order, bdd_index_pre);
        }

        inline std::vector<BDD> getBDDVarsEff() const {
            return getBDDVars(var_order, bdd_index_eff);
        }

        inline std::vector<BDD> getBDDVarsPre(const std::vector<int> &vars) const {
            return getBDDVars(vars, bdd_index_pre);
        }

        inline std::vector<BDD> getBDDVarsEff(const std::vector<int> &vars) const {
            return getBDDVars(vars, bdd_index_eff);
        }

        inline BDD validStates() const {
            return validBDD;
        }

        inline BDD bddVar(int index) const {
            return variables[index];
        }

        std::vector<int> getStateDescription(const std::vector<char> &binary_state) const;

        std::vector<int> sample_state(const BDD &bdd) const;


        template<class T>
        int *getBinaryDescription(const T &state) {
            int pos = 0;
            //  cout << "State " << endl;
            for (int v: var_order) {
                //cout << v << "=" << state[v] << " " << g_variable_domain[v] << " assignments and  " << binary_len[v] << " variables   " ;
                //preconditionBDDs[v] [state[v]].PrintMinterm();

                for (size_t j = 0; j < bdd_index_pre[v].size(); j++) {
                    binState[pos++] = ((state[v] >> j) % 2);
                    binState[pos++] = 0; //Skip interleaving variable
                }
            }
            /*std::cout << "Binary description: ";
               for(int i = 0; i < pos; i++){

                   if (i % 2 == 1) {
                       std::cout << "-";
                       assert(binState[i] == 0);
                   } else {
                       std::cout << binState[i];
                   }
               }
               std::cout << std::endl;*/

            return &(binState[0]);
        }

        void print_options() const;


    private:
        //Auxiliar function helping to create precondition and effect BDDs
        //Generates value for bddVars.
        BDD generateBDDVar(const std::vector<int> &_bddVars, int value) const;

        BDD getCube(int var, const std::vector<std::vector<int>> &v_index) const;

        BDD getCube(const std::set<int> &vars, const std::vector<std::vector<int>> &v_index) const;

        BDD createBiimplicationBDD(const std::vector<int> &vars, const std::vector<int> &vars2) const;

        std::vector<BDD> getBDDVars(const std::vector<int> &vars, const std::vector<std::vector<int>> &v_index) const;


        inline BDD createPreconditionBDD(int variable, int value) const {
            return generateBDDVar(bdd_index_pre[variable], value);
        }

        inline BDD createEffectBDD(int variable, int value) const {
            return generateBDDVar(bdd_index_eff[variable], value);
        }

        inline int getNumBDDVars() const {
            return numBDDVars;
        }
    };
}

#endif
