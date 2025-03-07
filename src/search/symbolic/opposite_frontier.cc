#include "opposite_frontier.h"

#include "sym_solution.h"

#include "../utils/logging.h"

using namespace std;

namespace symbolic {
    OppositeFrontierFixed::OppositeFrontierFixed(const std::shared_ptr<SymStateSpaceManager> &mgr, bool fw)
            : mgr (mgr), goal(fw ? mgr->getGoal() : mgr->getInitialState()), hNotGoal(mgr->getAbsoluteMinTransitionCost()) {
    }

    std::optional<SymSolution> OppositeFrontierFixed::checkCut(const std::shared_ptr<ClosedList> & closed, const BDD &states, int g, bool fw) const {
        BDD cut = states * goal;
        if (cut.IsZero()) {
            return {};
        }

        if (fw) //Solution reconstruction will fail
            return SymSolution(mgr, closed, nullptr, g, 0, cut);
        else
            return SymSolution(mgr, nullptr, closed, 0, g, cut);
    }

    BDD OppositeFrontierFixed::notClosed() const {
        return !goal;
    }

    int OppositeFrontierFixed::getGNotClosed() const {
        return hNotGoal;
    }

    OppositeFrontierNonStop::OppositeFrontierNonStop(const std::shared_ptr<SymStateSpaceManager> &mgr) : goal(mgr->zeroBDD()),
    hNotGoal(mgr->getAbsoluteMinTransitionCost()) {
    }

    std::optional<SymSolution>
    OppositeFrontierNonStop::checkCut(const std::shared_ptr<ClosedList> &, const BDD &, int , bool ) const {
        return {};
    }

    std::optional<SymSolution> OppositeFrontierClosed::checkCut(const std::shared_ptr<ClosedList> & other_closed, const BDD &states, int g, bool fw) const {
        BDD cut_candidate = states * closed->getClosedTotal();
        if (cut_candidate.IsZero()) {
            // TODO: Check with generated but not closed states?
            return {}; //No solution yet :(
        }

        for (const auto &closedH: closed->get_closed()) {
            int h = closedH.first;

            //DEBUG_MSG(cout << "Check cut of g=" << g << " with h=" << h << endl;);
            BDD cut = closedH.second * cut_candidate;
            if (!cut.IsZero()) {
                if (fw) {
                    return SymSolution(closed->getStateSpaceShared(), other_closed, closed, g, h, cut);
                } else {
                    return SymSolution(closed->getStateSpaceShared(), closed, other_closed, h, g, cut);
                }
            }
        }

        cerr << "Error: Cut with closedTotal but not found on closed" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }


    inline BDD OppositeFrontierClosed::notClosed() const {
        return closed->notClosed();
    }

    inline int OppositeFrontierClosed::getGNotClosed() const {
        return closed->getGNotClosed();
    }


}
