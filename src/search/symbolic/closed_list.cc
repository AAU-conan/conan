#include "closed_list.h"

#include "bdd_manager.h"
#include "sym_state_space_manager.h"
#include "transition_relation.h"
#include "sym_solution.h"
#include "uniform_cost_search.h"

#include <iostream>
#include <string>
#include <cassert>
#include <fcntl.h>
#include<ranges>

using namespace std::ranges;
using namespace std;

namespace symbolic {

    ClosedList::ClosedList(std::shared_ptr<SymStateSpaceManager> mgr): mgr(mgr), closedTotal(mgr->zeroBDD()) {
    }

    void ClosedList::init(const BDD &init, int _gNotGenerated) {
        gNotGenerated = _gNotGenerated;

        closedTotal = init;

        if (mgr->hasTransitions0()) {
            zeroCostClosed[0].push_back(init);
        } else {
            closed_states[0] = init;
            closedUpTo[0] = init;
        }
    }

    BDD ClosedList::remove_duplicates(const BDD &S) const {
        BDD SwoDuplicates = S * !closedTotal;
        // TODO: Here we should be pruning dead states (or perhaps this should be done before expansion
        // mgr->filterDead(frontier, fw, maxTime, maxNodes);
        return SwoDuplicates;
    }

    void ClosedList::reclose_states(int g, const BDD &S) {
        // if (log.is_at_least_verbose()) {
        //     log << *(mgr) << " " << (my_search->isFW() ? "fw" : "bw")
        //                          << " closing g= " << g << endl;
        // }

        if (closed_states.contains(g)) {
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

    void ClosedList::close_states(int g, const BDD &S, OpenList &open_list) {
        // if (log.is_at_least_verbose()) {
        //     log << *(my_search->getStateSpace()) << " " << (my_search->isFW() ? "fw" : "bw")
        //                          << " closing g= " << g << endl;
        // }

        BDD SwoDuplicates = remove_duplicates(S);

        if (closed_states.contains(g)) {
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
        open_list.insert_cost(g, SwoDuplicates);
    }


    // These states have been closed, but we have not applied zero cost operators yet.
    // Do not put them in closed. But put them in closedTotal to ensure that we remove duplicates.
    void ClosedList::close_states_zero(int g, const BDD &S, OpenList & open_list) {
        assert(mgr->hasTransitions0());

        // if (log.is_at_least_debug()) {
        //     log << *(my_search->getStateSpace()) << " " << (my_search->isFW() ? "fw" : "bw")
        //                          << " closing 0-cost actions with g=" << g << endl;
        // }

        BDD SwoDuplicates = remove_duplicates(S);

        zeroCostClosed[g].push_back(SwoDuplicates);

        //For the following operations we choose the smallest BDD, as the result should be the same anyway
        const BDD *smaller = SwoDuplicates.nodeCount() < S.nodeCount() ? &SwoDuplicates : &S;
        assert (closedTotal + SwoDuplicates == closedTotal + S);
        closedTotal += *smaller;
        open_list.insert_zero(g, SwoDuplicates);
    }

    void ClosedList::put_in_frontier(int g, const BDD &S) {
        generated_states[g].push_back(S);
    }

    std::optional<int> ClosedList::min_value_to_expand() const {
        for (auto &[cost, _]: zeroCostClosed | ranges::views::reverse) {
            if (!closed_states.contains(cost)) {
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
                if (!generated_states.contains(cost)) {
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
                    close_states_zero(cost, states, open_list);
                } else {
                    close_states(cost, states, open_list);
                }
            }
        }
    }

    void ClosedList::extract_path(const BDD &c, int h, bool fw, vector <OperatorID> &path) const {
        if (!mgr) return;

        // if (log.is_at_least_debug()) {
        //     log << "Sym closed extract path h=" << h << " gNotGenerated: " << gNotGenerated
        //                          << ", Closed: ";
        //     for (auto &tmp: closed_states) log << tmp.first << " ";
        //     log << endl;
        // }
        //
        const auto &trs = mgr->getIndividualTRs();
        BDD cut = c;
        size_t steps0 = 0;
        if (zeroCostClosed.contains(h)) {
            assert(trs.contains(0));
            while (steps0 < zeroCostClosed.at(h).size() && bdd_intersection_empty(cut, zeroCostClosed.at(h)[steps0])) {
                steps0++;
            }
            //DEBUG_MSG(cout << "Steps0 of h=" << h << " is " << steps0 << endl;);
            if (steps0 < zeroCostClosed.at(h).size()) {
                cut *= zeroCostClosed.at(h)[steps0];
            } else {
                //DEBUG_MSG(cout << "cut not found with steps0. Try to find with preimage: " << trs.contains(0) << endl;);
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
            // if (log.is_at_least_debug()) {
            //     log << "g=" << h << " and steps0=" << steps0 << endl;
            // }

            if (steps0 > 0) {
                bool foundZeroCost = false;

                //Apply 0-cost operators
                if (trs.contains(0)) {
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
                                // if (log.is_at_least_debug()) {
                                //     log << "Adding " << (*(tr->getOps().begin())).get_index() << endl;
                                // }
                                path.push_back(*(tr->getOps().begin()));
                                foundZeroCost = true;
                                break;
                            }
                        }
                    }
                }

                if (!foundZeroCost) {
                    // if (log.is_at_least_debug()) {
                    //     log << "didn't find" << endl;
                    // }

                    steps0 = 0;
                }
            }

            // if (log.is_at_least_debug()) {
            //     log << "g=" << h << " and steps0=" << steps0 << endl;
            // }

            if (h > 0 && steps0 == 0) {
                bool found = false;
                for (const auto &key: trs | ranges::views::reverse) { // We use the inverse order, so that more expensive actions are used first
                    if (found) break;
                    int newH = h - key.first;
                    for (auto &tr: key.second) {
                        //log << "Check " << " " << my_search->getStateSpaceShared()->getVars()->getTask().get_operator_name((*(tr->getOps().begin())).get_index(), false) << " of cost " << key.first << " in h=" << newH << endl;
                        BDD succ = tr->image(!fw, cut);

                        /*DEBUG_MSG(cout << "Image computed: "; succ.print(0,1); cout << "closed_states at newh: "; closed_states.at(newH).print(0,1););*/
                        if (!bdd_intersection_empty(succ, closed_states.at(newH))) {
                            h = newH;
                            cut = succ;
                            steps0 = 0;
                            if (zeroCostClosed.contains(h)) {
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
                    if (key.first == 0 || !closed_states.contains(newH)) continue;
                }
                if (!found) {
                    cerr << "Error: Solution reconstruction failed: " << endl;
                    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
                }
            }
        }
    }

    ADD ClosedList::getHeuristic() const {
        BDD statesWithHNotClosed = !closedTotal;
        ADD h = mgr->getVars()->get_bdd_manager()->getADD(0);
        //cout << "New heuristic with h [";
        for (auto &[h_val, closed_bdd]: closed_states) {
            //cout << it.first << " ";

            if (h_val != gNotGenerated) {
                h = h.Maximum(closed_bdd.Add() * mgr->getVars()->get_bdd_manager()->getADD(h_val));
            } else {
                statesWithHNotClosed += closed_bdd;
            }
        }

        if (gNotGenerated > 0 && !statesWithHNotClosed.IsZero()) {
            h = h.Maximum(statesWithHNotClosed.Add() * mgr->getVars()->get_bdd_manager()->getADD(gNotGenerated));
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
