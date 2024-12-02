#include "sym_variables.h"

#include "../abstract_task.h"
#include "../plugins/options.h"
#include "../plugins/plugin.h"
#include "../task_proxy.h"
#include "../variable_ordering/variable_ordering_strategy.h"

#include <ranges>
#include <utility>

using namespace std;
using plugins::Options;
using variable_ordering::VariableOrderingStrategy;

namespace symbolic {
    SymVariables::SymVariables(std::shared_ptr<BDDManager> manager, std::vector<int> var_order,
        std::vector<int> variable_domain_sizes) : bdd_manager(std::move(manager)), var_order(std::move(var_order)), variable_domain_sizes(variable_domain_sizes) {
        constructor();
    }

    SymVariables::SymVariables(shared_ptr <BDDManager> manager,
                               const VariableOrderingStrategy &variable_ordering,
                               shared_ptr <AbstractTask> task) :
            bdd_manager(std::move(manager)), var_order (variable_ordering.compute_variable_ordering(*task)) {

        variable_domain_sizes.reserve(var_order.size());
        for (size_t i = 0; i < var_order.size(); i++) {
            variable_domain_sizes.push_back(task->get_variable_domain_size(i));
        }
        constructor();
    }


    void SymVariables::constructor() {
        size_t num_fd_vars = var_order.size();
        //Initialize binary representation of variables.
        numBDDVars = 0;
        bdd_index_pre = vector<vector<int>>(var_order.size());
        bdd_index_eff = vector<vector<int>>(var_order.size());

        for (int var: var_order) {
            int var_len = static_cast<int>(ceil(log2(variable_domain_sizes[var])));
            for (int j = 0; j < var_len; j++) {
                bdd_index_pre[var].push_back(numBDDVars);
                bdd_index_eff[var].push_back(numBDDVars + 1);
                numBDDVars += 2;
            }
        }
        utils::g_log << "Symbolic Variables: " << var_order.size() << " => " << numBDDVars / 2 << " * 2 = "
                     << numBDDVars << ", variable order: ";
        for (int v: var_order)
            utils::g_log << v << " ";
        utils::g_log << endl;

        //TODO: Manager should be initialized only once; so probably this should be somewhere else.
        bdd_manager->init(numBDDVars);
        if (utils::g_log.is_at_least_debug()) {
            utils::g_log << "Generating binary variables" << endl;
        }
        //Generate binary_variables
        for (int i = 0; i < numBDDVars; i++) {
            variables.push_back(bdd_manager->bddVar(i));
        }

        //DEBUG_MSG(cout << "Generating predicate BDDs: " << num_fd_vars << endl;);
        preconditionBDDs.resize(num_fd_vars);
        effectBDDs.resize(num_fd_vars);
        biimpBDDs.resize(num_fd_vars);
        validValues.resize(num_fd_vars);
        validBDD = bdd_manager->oneBDD();
        //Generate predicate (precondition (s) and effect (s')) BDDs
        for (int var: var_order) {
            for (int j = 0; j < variable_domain_sizes[var]; j++) {
                preconditionBDDs[var].push_back(createPreconditionBDD(var, j));
                effectBDDs[var].push_back(createEffectBDD(var, j));
            }
            validValues[var] = bdd_manager->zeroBDD();
            for (int j = 0; j < variable_domain_sizes[var]; j++) {
                validValues[var] += preconditionBDDs[var][j];
            }
            validBDD *= validValues[var];
            biimpBDDs[var] = createBiimplicationBDD(bdd_index_pre[var], bdd_index_eff[var]);
        }

        binState.resize(numBDDVars, 0);
        binStateChar.resize(numBDDVars, 0);
        if (utils::g_log.is_at_least_debug()) {
            utils::g_log << "Symbolic Variables... Done." << endl;
        //     for(int i = 0; i < variable_domain_sizes.size(); i++){
        //         for(int j = 0; j < variable_domain_sizes[i]; j++){
        //             //cout << "Var-val: " << i << "-" << j << endl;
        //             //preconditionBDDs[i][j].print(1,2);
        //             //effectBDDs[i][j].print(1,2);
        //         }
        //     }
        }
    }
    BDD SymVariables::getStateBDD(const State &state) const {
        assert(state.size() == var_order.size());
        BDD res = bdd_manager->oneBDD();
        for (int i = var_order.size() - 1; i >= 0; i--) {
            size_t variable = var_order[i];
            int value = state[variable].get_value();
            res = res * preconditionBDDs[variable][value];
        }
        return res;
    }

    BDD SymVariables::getStateBDD(const std::vector<int> &state) const {
        BDD res = bdd_manager->oneBDD();
        for (int i = var_order.size() - 1; i >= 0; i--) {
            res = res * preconditionBDDs[var_order[i]][state[var_order[i]]];
        }
        return res;
    }

// State SymVariables::getStateFrom(const BDD & bdd) const {
//   vector <int> vals;
//   BDD current = bdd;
//   for(int var = 0; var < g_variable_domain.size(); var++){
//     for(int val = 0; val < g_variable_domain[var]; val++){
//       BDD aux = current*preconditionBDDs[var][val];
//       if(!aux.IsZero()){
//      current = aux;
//      vals.push_back(val);
//      break;
//       }
//     }
//   }
//   return State(vals);
// }


    BDD SymVariables::getPartialStateBDD(const vector <pair<int, int>> &state) const {
        BDD res = validBDD;
        for (int i = state.size() - 1; i >= 0; i--) {
            res = res * preconditionBDDs[state[i].first][state[i].second];
        }
        return res;
    }

    BDD SymVariables::getPartialStateBDD(const vector <FactPair> &state) const {
        BDD res = validBDD;
        for (int i = state.size() - 1; i >= 0; i--) {
            res = res * preconditionBDDs[state[i].var][state[i].value];
        }
        return res;
    }


    bool SymVariables::isIn(const State &state, const BDD &bdd) const {
        //TODO: Replace by efficient variant
        BDD sBDD = getStateBDD(state);
        return !((sBDD * bdd).IsZero());
    }

    double SymVariables::numStates(const BDD &bdd) const {
        return bdd.CountMinterm(numBDDVars/2);
    }

    double SymVariables::numStates() const {
        return numStates(validBDD);
    }

    double SymVariables::numStates(const DisjunctiveBucket &bucket) const {
        double sum = 0;
        for (const BDD &bdd: bucket.bucket) {
            sum += numStates(bdd);
        }
        return sum;
    }


    BDD SymVariables::generateBDDVar(const std::vector<int> &_bddVars, int value) const {
        BDD res = bdd_manager->oneBDD();
        for (int v: _bddVars) {
            if (value % 2) { //Check if the binary variable is asserted or negated
                res = res * variables[v];
            } else {
                res = res * (!variables[v]);
            }
            value /= 2;
        }
        return res;
    }

    BDD SymVariables::createBiimplicationBDD(const std::vector<int> &vars, const std::vector<int> &vars2) const {
        BDD res = bdd_manager->oneBDD();
        for (size_t i = 0; i < vars.size(); i++) {
            res *= variables[vars[i]].Xnor(variables[vars2[i]]);
        }
        return res;
    }

    vector <BDD> SymVariables::getBDDVars(const vector<int> &vars, const vector <vector<int>> &v_index) const {
        vector<BDD> res;
        for (int v: vars) {
            for (int bddv: v_index[v]) {
                res.push_back(variables[bddv]);
            }
        }
        return res;
    }


    BDD SymVariables::getCube(int var, const vector <vector<int>> &v_index) const {
        BDD res = bdd_manager->oneBDD();
        for (int bddv: v_index[var]) {
            res *= variables[bddv];
        }
        return res;
    }

    BDD SymVariables::getCube(const set<int> &vars, const vector <vector<int>> &v_index) const {
        BDD res = bdd_manager->oneBDD();
        for (int v: vars) {
            for (int bddv: v_index[v]) {
                res *= variables[bddv];
            }
        }
        return res;
    }

    vector<int> SymVariables::sample_state(const BDD &bdd) const {
        bdd.PickOneCube(&(binStateChar[0]));
        return getStateDescription(binStateChar);
    }


    std::vector<int> SymVariables::getStateDescription(const vector<char> &binary_state) const {
        vector<int> state(var_order.size(), 0);

        for (int v: var_order) {
            for (int bdd_v : bdd_index_pre[v] | std::views::reverse) {
                assert (binary_state[bdd_v] == 0 || binary_state[bdd_v] == 1 || binary_state[bdd_v] == 2);

                state[v] *= 2;
                if(binary_state[bdd_v] == 1) {
                    state[v] += binary_state[bdd_v];
                }
            }

            assert (state[v] < (int)(preconditionBDDs[v].size()));
        }

        return state;
    }

}
