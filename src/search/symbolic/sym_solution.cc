#include "sym_solution.h"

#include <vector>       // std::vector
#include "../state_registry.h"
#include "../task_utils/task_properties.h"

#include "unidirectional_search.h"


using namespace std;

namespace symbolic {

    void SymSolution::getPlan(vector <OperatorID> &path) const {
        assert (path.empty()); //This code should be modified to allow appending things to paths
        //DEBUG_MSG(cout << "Extract path forward: " << g << endl; );

        BDD newCut;
        if (exp_fw) {
            exp_fw->getPlan(cut, g, path);
            TaskProxy task(exp_fw->getStateSpace()->getVars()->getTask());
            State s = task.get_initial_state();

            if (!path.empty()) {
                //Get state
                for (auto op_id: path) {
                    auto op = task.get_operators()[op_id];
                    assert (task_properties::is_applicable(op, s));
                    s = s.get_unregistered_successor(op);
                }
            }
            newCut = exp_fw->getStateSpace()->getVars()->getStateBDD(s);
        } else {
            newCut = cut;
        }

        if (exp_bw) {
            exp_bw->getPlan(newCut, h, path);
        }
    }

    ADD SymSolution::getADD() const {
        assert(exp_fw || exp_bw);
        vector<OperatorID> path;
        getPlan(path);

        SymVariables *vars = nullptr;
        if (exp_fw) vars = exp_fw->getStateSpace()->getVars();
        else if (exp_bw) vars = exp_bw->getStateSpace()->getVars();

        ADD hADD = vars->get_bdd_manager()->getADD(-1);
        int h_val = g + h;

        TaskProxy task(
                exp_fw ? exp_fw->getStateSpace()->getVars()->getTask() : exp_bw->getStateSpace()->getVars()->getTask());
        State s = task.get_initial_state();
        BDD sBDD = vars->getStateBDD(s);
        hADD += sBDD.Add() * (vars->get_bdd_manager()->getADD(h_val + 1));
        for (auto op_id: path) {
            const auto &op = task.get_operators()[op_id];
            h_val -= op.get_cost();
            s = s.get_unregistered_successor(op);
            sBDD = vars->getStateBDD(s);
            hADD += sBDD.Add() * (vars->get_bdd_manager()->getADD(h_val + 1));
        }
        return hADD;
    }

}
