#ifndef SYMBOLIC_UNIDIRECTIONAL_SEARCH_H
#define SYMBOLIC_UNIDIRECTIONAL_SEARCH_H

#include "sym_search.h"
#include "../algorithms/step_time_estimation.h"
#include "merge_bdds.h"
#include "../operator_id.h"
#include <vector>
#include <map>
#include <memory>

namespace symbolic {
    class SymSolution;

    class UnidirectionalSearch;

    class SymController;

    class ClosedList;

    class SymExpStatistics {
    public:
        double image_time, image_time_failed;
        double time_heuristic_evaluation;
        int num_steps_succeeded;
        double step_time;

        SymExpStatistics() :
                image_time(0),
                image_time_failed(0), time_heuristic_evaluation(0),
                num_steps_succeeded(0), step_time(0) {}


        void add_image_time(double t) {
            image_time += t;
            num_steps_succeeded += 1;
        }

        void add_image_time_failed(double t) {
            image_time += t;
            image_time_failed += t;
            num_steps_succeeded += 1;
        }
    };


    class OppositeFrontier {
    public:
        virtual ~OppositeFrontier() = default;

        virtual SymSolution checkCut(UnidirectionalSearch *search, const BDD &states, int g, bool fw) const = 0;

        virtual BDD notClosed() const = 0;

        //Returns true only if all not closed states are guaranteed to be dead ends
/*
        virtual bool exhausted() const = 0;
*/
        virtual int getGNotClosed() const = 0;
    };

    class OppositeFrontierFixed : public OppositeFrontier {
        BDD goal;
        int hNotGoal;
    public:
        OppositeFrontierFixed(BDD g, const SymStateSpaceManager &mgr);

        virtual SymSolution checkCut(UnidirectionalSearch *search, const BDD &states, int g, bool fw) const override;

        virtual BDD notClosed() const override {
            return !goal;
        }

/*
        virtual bool exhausted() const override {
            return false;
        }

*/
        int getGNotClosed() const override {
            return hNotGoal;
        }

        virtual ~OppositeFrontierFixed() = default;
    };


    class UnidirectionalSearch : public SymSearch {
    protected:
        bool fw; //Direction of the search. true=forward, false=backward

        SymExpStatistics stats;

        std::shared_ptr<OppositeFrontier> perfectHeuristic;
        std::shared_ptr<ClosedList> closed = nullptr;

    public:
        UnidirectionalSearch(SymController *eng, const SymParamsSearch &params);

        inline bool isFW() const {
            return fw;
        }

        void statistics() const;

        virtual void getPlan(const BDD &cut, int g, std::vector<OperatorID> &path) const = 0;

        virtual int getG() const = 0;

        virtual std::shared_ptr<ClosedList> getClosed() {
            return nullptr;
        }
    };
}
#endif
