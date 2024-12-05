#include "sym_solution.h"

#include <vector>

#include "closed_list.h"
#include "sym_state_space_manager.h"
#include "../state_registry.h"
#include "../task_utils/task_properties.h"

using namespace std;

namespace symbolic {

    void SymSolution::getPlan(vector <OperatorID> &path) const {
        assert (path.empty()); //This code should be modified to allow appending things to paths
        //DEBUG_MSG(cout << "Extract path forward: " << g << endl; );

        BDD newCut;
        if (exp_fw) {
            exp_fw->extract_path(cut, g, true, path);
            std::ranges::reverse(path);

            TaskProxy task = mgr->getTask();
            State s = task.get_initial_state();

            if (!path.empty()) {
                //Get state
                for (auto op_id: path) {
                    auto op = task.get_operators()[op_id];
                    assert (task_properties::is_applicable(op, s));
                    s = s.get_unregistered_successor(op);
                }
            }
            newCut = mgr->getVars()->getStateBDD(s);
        } else {
            newCut = cut;
        }

        if (exp_bw) {
            exp_bw->extract_path(newCut, h, false, path);
        }
    }

    ADD SymSolution::getADD() const {
        assert(exp_fw || exp_bw);
        vector<OperatorID> path;
        getPlan(path);

        SymVariables *vars = mgr->getVars();

        ADD hADD = vars->get_bdd_manager()->getADD(-1);
        int h_val = g + h;

        TaskProxy task = mgr->getTask();
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

    void SymSolutionLowerBound::new_solution(const SymSolution &sol) {
        if (!solution || sol.getCost() < solution->getCost()) {
            solution = sol;
            //TODO: Remove from here or at least take some other log object
            utils::g_log << "BOUND: " << lower_bound << " < " << getUpperBoundString() << std::endl;

            for (auto notifier: notifiers) {
                notifier->notify_solution(sol);
            }
        }
    }

    void SymSolutionLowerBound::setLowerBound(int lower) {
        // Never set a lower bound greater than the current upper bound
        if (solution) {
            lower = min(lower, solution->getCost());
        }

        if (lower > lower_bound) {
            lower_bound = lower;
            //TODO: Remove from here or at least take some other log object
            utils::g_log << "BOUND: " << lower_bound << " < " << getUpperBoundString() << std::endl;
        }
    }

    std::string SymSolutionLowerBound::getUpperBoundString() const {
        if (solution) return std::to_string(solution->getCost());
        else return "infinity";
    }
}
