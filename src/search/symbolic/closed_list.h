#ifndef SYMBOLIC_CLOSED_LIST_H
#define SYMBOLIC_CLOSED_LIST_H

#include "sym_variables.h"

#include <vector>
#include <set>
#include <map>

class OperatorID;

namespace symbolic {

    class SymStateSpaceManager;

    class SymSolution;

    class UnidirectionalSearch;

    class SymSearch;

    class OpenList;
/*
 * The closed list contains all states for which we know the exact distance (or an upperbound thereof)
 * from the set of initial states of the corresponding search. States in the frontier have already been generated,
 * but have not been "cleaned up" by aggregating multiple sets of states together.
 * Also, they may have been reached with a suboptimal g-value.
 *
 * A state is considered "closed" when we know the exact distance.
 * This is unrelated to whether the state has been expanded or not.
 */
    class ClosedList {
    private:
        std::shared_ptr<SymStateSpaceManager> mgr;  //Symbolic manager to perform bdd operations

        BDD closedTotal;  // All closed states.
        std::map<int, BDD> closed_states;   // Mapping from cost to set of states
        // For all states in closed, we are always sure that this is the exact distance from the initial set of states of the search

        // The frontier is the current states that have been generated but not closed.
        // The reason we wait to close them is because (a) there are other states to be generated with
        // the same cost, so we assign all not closed states the same g-value, so we don't gain information
        // from closing these states;
        // and (b) by waiting we can filter and merge all states at the same time.
        // We need to be careful of also including these states when checking against the other frontier
        std::map<int, DisjunctiveBucket> generated_states;

        // Auxiliar BDDs for the number of 0-cost action steps
        // Warning: The information here is not the exact number of 0-cost action steps,
        // but just an over-estimation to extract a path more quickly
        std::map<int, std::vector<BDD>> zeroCostClosed;

        int gNotGenerated = 0; // Bounds on g value for states that have not been generated
        std::map<int, BDD> closedUpTo;  // Disjunction of BDDs in closed  (auxiliar useful to take the maximum between several BDDs)

        void close_states(int g, const BDD &S, OpenList& open_list);
        void close_states_zero(int g, const BDD &S, OpenList& open_list);
        void reclose_states(int g, const BDD &S);

        [[nodiscard]] BDD remove_duplicates (const BDD & S) const;

        std::optional<int> min_value_to_expand() const;

        public:
        explicit ClosedList(const std::shared_ptr<SymStateSpaceManager>& mgr);

        void init(const BDD &init, int gNotGenerated);

        void put_in_frontier(int g, const BDD &S);

        void closeUpTo(OpenList & open, utils::Duration maxTime, long maxNodes);

        const BDD & getClosedTotal() const {
            return closedTotal;
        }

        BDD notClosed() const {
            return !closedTotal;
        }

        const std::map<int, BDD> & get_closed() const {
            return closed_states;
        }

        BDD get_closed(int g) const {
            return closed_states.at(g);
        }

        int getGNotClosed() const {
            if (generated_states.empty()) {
                return gNotGenerated;
            } else {
                return std::min(gNotGenerated, generated_states.begin()->first);
            }
        }

        void extract_path(const BDD &cut, int h, bool fw, std::vector<OperatorID> &path) const;

        ADD getHeuristic() const;

        void getHeuristic(std::vector<ADD> &heuristics, std::vector<int> &maxHeuristicValues) const;

        void statistics() const;

        double average_value() const;

        const std::shared_ptr<SymStateSpaceManager> & getStateSpaceShared() const {
            return mgr;
        }
    };
}

#endif