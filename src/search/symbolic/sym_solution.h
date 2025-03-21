#ifndef SYMBOLIC_SYM_SOLUTION_H
#define SYMBOLIC_SYM_SOLUTION_H

#include "sym_variables.h"
#include <utility>
#include <vector>

#include "../utils/logging.h"

class OperatorID;

namespace symbolic {
    class ClosedList;
    class SymStateSpaceManager;

    class SymSolution {
        std::shared_ptr<SymStateSpaceManager> mgr;
        std::shared_ptr<ClosedList> exp_fw, exp_bw;
        int g, h;
        BDD cut;

    public:
        SymSolution(std::shared_ptr<SymStateSpaceManager> mgr, std::shared_ptr<ClosedList> exp_fw,
                    std::shared_ptr<ClosedList> exp_bw, int g_val, int h_val, const BDD& S) : mgr(std::move(std::move(mgr))),
            exp_fw(std::move(exp_fw)), exp_bw(std::move(exp_bw)), g(g_val), h(h_val), cut(S) {
        }

        void getPlan(std::vector<OperatorID> &path) const;

        ADD getADD() const;

        int getCost() const {
            return g + h;
        }
    };

    class SolutionNotifier {
    public:
        virtual ~SolutionNotifier() = default;

        virtual void notify_solution(const SymSolution &sol) = 0;
    };

    class SymSolutionLowerBound {
        int lower_bound = 0;
        std::optional<SymSolution> solution;
        std::vector<SolutionNotifier *> notifiers;

        std::string getUpperBoundString() const;

    public:

        void add_notifier(SolutionNotifier *notifier) {
            notifiers.push_back(notifier);
        }

        int getUpperBound() const;

        int getLowerBound() const {
            return lower_bound;
        }

        bool solved() const {
            return getLowerBound() >= getUpperBound();
        }

        const std::optional<SymSolution> & get_solution() const {
            return solution;
        }

        void new_solution(const SymSolution &sol, utils::LogProxy &log);

        void setLowerBound(int lower, utils::LogProxy &log);
    };
}
#endif
