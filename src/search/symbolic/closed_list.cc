#include "closed_list.h"

#include "bdd_manager.h"
#include "sym_state_space_manager.h"
#include "transition_relation.h"
#include "sym_solution.h"
#include "uniform_cost_search.h"
#include "../abstract_task.h"

#include <iostream>
#include <string>
#include <cassert>
#include<ranges>

using namespace std::ranges;
using namespace std;

namespace symbolic {
    void
    ClosedList::init(SymStateSpaceManager *manager, UnidirectionalSearch *search, const BDD &init, int _gNotGenerated) {
        mgr = manager;
        my_search = search;

        closed_states.clear();
        generated_states.clear();
        zeroCostClosed.clear();
        closedUpTo.clear();
        closedTotal = mgr->zeroBDD();
        gNotGenerated = _gNotGenerated;

        if (mgr->hasTransitions0()) {
            close_states_zero(0, init);
        } else {
            close_states(0, init);
        }
    }

    BDD ClosedList::remove_duplicates(const BDD &S) const {
        BDD SwoDuplicates = S * !closedTotal;
        // TODO: Here we should be pruning dead states (or perhaps this should be done before expansion
        // mgr->filterDead(frontier, fw, maxTime, maxNodes);
        return SwoDuplicates;
    }

    void ClosedList::reclose_states(int g, const BDD &S) {
        if (my_search->get_log().is_at_least_verbose()) {
            my_search->get_log() << *(my_search->getStateSpace()) << " " << (my_search->isFW() ? "fw" : "bw")
                                 << " closing g= " << g << endl;
        }

        if (closed_states.count(g)) {
            closed_states[g] += S;
        } else {
            closed_states[g] = S;
        }

        //Introduce in closedUpTo
        auto c = closedUpTo.lower_bound(g);
        while (c != std::end(closedUpTo)) {
            c->second += S;
            c++;
        }
    }

    void ClosedList::close_states(int g, const BDD &S) {
        if (my_search->get_log().is_at_least_verbose()) {
            my_search->get_log() << *(my_search->getStateSpace()) << " " << (my_search->isFW() ? "fw" : "bw")
                                 << " closing g= " << g << endl;
        }

        BDD SwoDuplicates = remove_duplicates(S);

        if (closed_states.count(g)) {
            closed_states[g] += SwoDuplicates;
        } else {
            closed_states[g] = SwoDuplicates;
        }

        //For the following operations we choose the smallest BDD, as the result should be the same anyway
        const BDD *smaller = SwoDuplicates.nodeCount() < S.nodeCount() ? &SwoDuplicates : &S;
        assert (closedTotal + SwoDuplicates == closedTotal + S);

        closedTotal += *smaller;

        //Introduce in closedUpTo
        auto c = closedUpTo.lower_bound(g);
        while (c != std::end(closedUpTo)) {
            c->second += *smaller;
            c++;
        }
    }


    // These states have been closed, but we have not applied zero cost operators yet.
    // Do not put them in closed. But put them in closedTotal to ensure that we remove duplicates.
    void ClosedList::close_states_zero(int g, const BDD &S) {
        assert(mgr->hasTransitions0());

        if (my_search->get_log().is_at_least_verbose()) {
            my_search->get_log() << *(my_search->getStateSpace()) << " " << (my_search->isFW() ? "fw" : "bw")
                                 << " closing 0-cost actions with g=" << g << endl;
        }

        BDD SwoDuplicates = remove_duplicates(S);

        zeroCostClosed[g].push_back(SwoDuplicates);

        //For the following operations we choose the smallest BDD, as the result should be the same anyway
        const BDD *smaller = SwoDuplicates.nodeCount() < S.nodeCount() ? &SwoDuplicates : &S;
        assert (closedTotal + SwoDuplicates == closedTotal + S);
        closedTotal += *smaller;
    }

    void ClosedList::put_in_frontier(int g, const BDD &S) {
        generated_states[g].push_back(S);
    }

    std::optional<int> ClosedList::min_value_to_expand() const {
        for (auto &[cost, _]: zeroCostClosed | views::reverse) {
            if (!closed_states.count(cost)) {
                assert (generated_states.empty() || generated_states.begin()->first >= cost);
                return cost;
            }
        }
        if (!generated_states.empty()) {
            return generated_states.begin()->first;
        }
        return std::nullopt;
    }

    void ClosedList::closeUpTo(OpenList &open_list, utils::Duration maxTime, long maxNodes) {
        int g_not_generated = open_list.minG();
        std::optional<int> zero_cost_to_expand;
        if (auto val = min_value_to_expand()) {
            g_not_generated = std::min<int>(g_not_generated, val.value() + mgr->getAbsoluteMinTransitionCost());
        }
        assert(g_not_generated >= gNotGenerated);
        if (g_not_generated > gNotGenerated) {
            gNotGenerated = g_not_generated;
        }

        // If there are 0-cost transitions, we may need to close them to apply 0-cost actions
        // Basically, we need to close those states in zeroCostClosed that are not yet in closed,
        // and that are not in generated
        if (mgr->hasTransitions0()) {
            if (auto val = min_value_to_expand()) {
                int cost = val.value();
                if (!generated_states.count(cost)) {
                    DisjunctiveBucket bucket{zeroCostClosed.at(cost)};
                    bucket.merge_bucket(mgr->getVars()->get_bdd_manager(), maxTime, maxNodes);

                    for (const BDD &states: bucket.bucket) {
                        reclose_states(cost, states);
                        open_list.insert_cost(cost, states);
                    }
                }
            }
        }
        // At this point we can close all generated states that have a value lower than gNotGenerated
        while (!generated_states.empty() &&
               generated_states.begin()->first <= gNotGenerated - (mgr->hasTransitions0() ? 0 : 1)) {
            auto f = generated_states.extract(generated_states.begin());
            int cost = f.key();

            DisjunctiveBucket bucket = f.mapped();
            bucket.merge_bucket(mgr->getVars()->get_bdd_manager(), maxTime, maxNodes);
            // Note that duplicates and dead states are removed inside closed_states

            for (const BDD &states: bucket.bucket) {
                if (mgr->hasTransitions0()) {
                    close_states_zero(cost, states);
                    open_list.insert_zero(cost, states);
                } else {
                    close_states(cost, states);
                    open_list.insert_cost(cost, states);
                }
            }
        }
    }

    void ClosedList::extract_path(const BDD &c, int h, bool fw, vector <OperatorID> &path) const {
        if (!mgr) return;

        if (my_search->get_log().is_at_least_debug()) {
            my_search->get_log() << "Sym closed extract path h=" << h << " gNotGenerated: " << gNotGenerated
                                 << ", Closed: ";
            for (auto &tmp: closed_states) my_search->get_log() << tmp.first << " ";
            my_search->get_log() << endl;
        }

        const auto &trs = mgr->getIndividualTRs();
        BDD cut = c;
        size_t steps0 = 0;
        if (zeroCostClosed.count(h)) {
            assert(trs.count(0));
            while (steps0 < zeroCostClosed.at(h).size() && bdd_intersection_empty(cut, zeroCostClosed.at(h)[steps0])) {
                steps0++;
            }
            //DEBUG_MSG(cout << "Steps0 of h=" << h << " is " << steps0 << endl;);
            if (steps0 < zeroCostClosed.at(h).size()) {
                cut *= zeroCostClosed.at(h)[steps0];
            } else {
                //DEBUG_MSG(cout << "cut not found with steps0. Try to find with preimage: " << trs.count(0) << endl;);
                bool foundZeroCost = false;
                for (const auto &tr: trs.at(0)) {
                    if (foundZeroCost)
                        break;
                    BDD succ = tr->image(!fw, cut);
                    if (succ.IsZero()) {
                        continue;
                    }

                    for (size_t newSteps0 = 0; newSteps0 < steps0; newSteps0++) {
                        BDD intersection = succ * zeroCostClosed.at(h)[newSteps0];
                        if (!intersection.IsZero()) {
                            steps0 = newSteps0;
                            cut = intersection;
                            //DEBUG_MSG(cout << "Adding " << (*(tr.getOps().begin()))->get_name() << endl;);
                            path.push_back(*(tr->getOps().begin()));
                            foundZeroCost = true;
                            break;
                        }
                    }
                }
                if (!foundZeroCost) {
                    //DEBUG_MSG(cout << "cut not found with steps0. steps0=0:" << endl;);
                    steps0 = 0;
                }
            }
        }

        while (h > 0 || steps0 > 0) {
            if (my_search->get_log().is_at_least_debug()) {
                my_search->get_log() << "g=" << h << " and steps0=" << steps0 << endl;
            }

            if (steps0 > 0) {
                bool foundZeroCost = false;

                //Apply 0-cost operators
                if (trs.count(0)) {
                    for (const auto &tr: trs.at(0)) {
                        if (foundZeroCost)
                            break;
                        BDD succ = tr->image(!fw, cut);
                        if (succ.IsZero()) {
                            continue;
                        }
                        for (size_t newSteps0 = 0; newSteps0 < steps0; newSteps0++) {
                            if (!bdd_intersection_empty(succ, zeroCostClosed.at(h)[newSteps0])) {
                                steps0 = newSteps0;
                                cut = succ;
                                if (my_search->get_log().is_at_least_debug()) {
                                    my_search->get_log() << "Adding " << (*(tr->getOps().begin())).get_index() << endl;
                                }
                                path.push_back(*(tr->getOps().begin()));
                                foundZeroCost = true;
                                break;
                            }
                        }
                    }
                }

                if (!foundZeroCost) {
                    if (my_search->get_log().is_at_least_debug()) {
                        my_search->get_log() << "didn't find" << endl;
                    }

                    steps0 = 0;
                }
            }

            if (my_search->get_log().is_at_least_debug()) {
                my_search->get_log() << "g=" << h << " and steps0=" << steps0 << endl;
            }

            if (h > 0 && steps0 == 0) {
                bool found = false;
                for (const auto &key: trs | views::reverse) { // We use the inverse order, so that more expensive actions are used first
                    if (found) break;
                    int newH = h - key.first;
                    if (key.first == 0 || !closed_states.count(newH)) continue;
                    for (auto &tr: key.second) {
                        //my_search->get_log() << "Check " << " " << my_search->getStateSpaceShared()->getVars()->getTask().get_operator_name((*(tr->getOps().begin())).get_index(), false) << " of cost " << key.first << " in h=" << newH << endl;
                        BDD succ = tr->image(!fw, cut);

                        /*DEBUG_MSG(cout << "Image computed: "; succ.print(0,1); cout << "closed_states at newh: "; closed_states.at(newH).print(0,1););*/
                        if (!bdd_intersection_empty(succ, closed_states.at(newH))) {
                            h = newH;
                            cut = succ;
                            steps0 = 0;
                            if (zeroCostClosed.count(h)) {
                                while (bdd_intersection_empty(cut, zeroCostClosed.at(h)[steps0])) {
                                    //DEBUG_MSG(cout << "r Steps0 is not " << steps0<< " of " << zeroCostClosed.at(h).size() << endl;);
                                    steps0++;
                                    assert(steps0 < zeroCostClosed.at(newH).size());
                                }
                                //DEBUG_MSG(cout << "r Steps0 of h=" << h << " is " << steps0 << endl;);
                                cut *= zeroCostClosed.at(h)[steps0];
                            }
                            path.push_back(*(tr->getOps().begin()));
                            //DEBUG_MSG(cout << "Selected " << path.back()->get_name() << endl;);
                            found = true;
                            break;
                        }
                    }
                }
                if (!found) {
                    cerr << "Error: Solution reconstruction failed: " << endl;
                    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
                }
            }
        }
    }

    SymSolution ClosedList::checkCut(UnidirectionalSearch *search, const BDD &states, int g, bool fw) const {
        BDD cut_candidate = states * closedTotal;
        if (cut_candidate.IsZero()) {
            // TODO: Check with generated but not closed states?
            return {}; //No solution yet :(
        }

        for (const auto &closedH: closed_states) {
            int h = closedH.first;

            //DEBUG_MSG(cout << "Check cut of g=" << g << " with h=" << h << endl;);
            BDD cut = closedH.second * cut_candidate;
            if (!cut.IsZero()) {
                if (fw)
                    return {search, my_search, g, h, cut};
                else
                    return {my_search, search, h, g, cut};
            }
        }

        cerr << "Error: Cut with closedTotal but not found on closed" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }

    void ClosedList::getHeuristic(vector <ADD> &heuristics,
                                  vector<int> &maxHeuristicValues) const {
        int previousMaxH = 0; //Get the previous value of max h
        if (!maxHeuristicValues.empty()) {
            previousMaxH = maxHeuristicValues.back();
        }
        /* If we did not complete one step, and we do not surpass the previous maxH we do not have heuristic*/
        if (closed_states.size() <= 1 && gNotGenerated <= previousMaxH) {
            cout << "Heuristic not inserted: "
                 << gNotGenerated << " " << closed_states.size() << endl;
            return;
        }

        ADD h = getHeuristic(previousMaxH);

        //  closed.clear(); //The heuristic values have been stored in the ADD.
        cout << "Heuristic with maxValue: " << gNotGenerated
             << " ADD size: " << h.nodeCount() << endl;

        maxHeuristicValues.push_back(gNotGenerated);
        heuristics.push_back(h);
    }


    ADD ClosedList::getHeuristic(int previousMaxH /*= -1*/) const {
        /* When zero cost operators have been expanded, all the states in non reached
           have a h-value strictly greater than frontierCost.
           They can be frontierCost + min_action_cost or the least bucket in open. */
        /*  int valueNonReached = frontierCost;
            if(frontierCost >= 0 && zeroCostExpanded){
            cout << "Frontier cost is " << frontierCost << endl;
            closed[frontierCost] = S;
            valueNonReached = frontierCost + mgr->getMinTransitionCost();
            if(!open.empty()){
            valueNonReached = min(open.begin()->first,
            valueNonReached);
            }
            }*/
        BDD statesWithHNotClosed = !closedTotal;
        ADD h = mgr->getVars()->get_bdd_manager()->getADD(-1);
        //cout << "New heuristic with h [";
        for (auto &it: closed_states) {
            //cout << it.first << " ";
            int h_val = it.first;

            /*If h_val < previousMaxH we can put it to that value
              However, we only do so if it is less than hNotClosed
              (or we will think that we have not fully determined the value)*/
            if (h_val < previousMaxH && previousMaxH < gNotGenerated) {
                h_val = previousMaxH;
            }
            if (h_val != gNotGenerated) {
                h += it.second.Add() * mgr->getVars()->get_bdd_manager()->getADD(h_val + 1);
            } else {
                statesWithHNotClosed += it.second;
            }
        }
        //cout << gNotGenerated << "]" << endl;

        if (gNotGenerated != numeric_limits<int>::max() && gNotGenerated >= 0 && !statesWithHNotClosed.IsZero()) {
            h += statesWithHNotClosed.Add() * mgr->getVars()->get_bdd_manager()->getADD(gNotGenerated + 1);
        }

        return h;
    }


    void ClosedList::statistics() const {
        // cout << "h (eval " << num_calls_eval << ", not_closed" << time_eval_states << "s, closed " << time_closed_states
        //   << "s, pruned " << time_pruned_states << "s, some " << time_prune_some
        //   << "s, all " << time_prune_all  << ", children " << time_prune_some_children << "s)";
    }

    double ClosedList::average_value() const {
        double averageHeuristic = 0;
        double heuristicSize = 0;
        for (const auto &item: closed_states) {
            double currentSize = mgr->getVars()->numStates(item.second);
            //DEBUG_MSG(cout << item.first << " " << currentSize << endl;);
            averageHeuristic += currentSize * item.first;
            heuristicSize += currentSize;
        }
        double notClosedSize = mgr->getVars()->numStates(notClosed());
        heuristicSize += notClosedSize;
        int maxH = (closed_states.empty() ? 0 : closed_states.rbegin()->first);

/*
        DEBUG_MSG(cout << maxH << " " << notClosedSize << endl;
                          cout << "Max size: " << heuristicSize << endl << endl;);
*/

        averageHeuristic += notClosedSize * maxH;
        return averageHeuristic / heuristicSize;
    }
}
