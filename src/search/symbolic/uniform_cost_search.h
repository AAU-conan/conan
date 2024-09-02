#ifndef SYMBOLIC_UNIFORM_COST_SEARCH_H
#define SYMBOLIC_UNIFORM_COST_SEARCH_H

#include "unidirectional_search.h"
#include "sym_state_space_manager.h"
#include "bucket.h"
#include "closed_list.h"
#include "merge_bdds.h"

#include "../algorithms/step_time_estimation.h"
#include "../algorithms/priority_queues.h"

#include <vector>
#include <map>
#include <memory>

namespace symbolic {

    // The search intercalates two types of steps:
    //
    // StepPrepareCost collects all BDDs that have been generated with a given cost,
    // filters out duplicates and mutexes so that we can close them and compute further images.
    // Then, introduces step images for those.
    //
    // StepImage computes an image
    //
    // Basically, among steps of the same g(), we prioritize step image, as that way we always prepare the cost only once.

    // In each step, we apply a transition relation to an action cost
    struct StepImage {
        int state_cost;
        BDD bdd;
        TransitionRelation *transition_relation;

        StepImage(int stateCost, const BDD &bdd, TransitionRelation *transitionRelation);

        int g() const;

        friend std::ostream &operator<<(std::ostream &os, const StepImage &step);
    };

    struct BDDWithCost {
        BDD bdd;
        int g;
    };

    class OpenList {
        std::vector<std::shared_ptr<TransitionRelation>> transition_relations_cost;
        std::vector<std::shared_ptr<TransitionRelation>> transition_relations_zero;

        std::map<int, std::vector<StepImage>> open;

    public:
        void init(const SymStateSpaceManager &mgr);

        bool empty() const;

        StepImage pop();

        int minG() const;

        void insert_zero(int state_cost, BDD bdd);
        void insert_cost(int state_cost, BDD bdd);

        void insert(StepImage step);


        friend std::ostream &operator<<(std::ostream &os, const OpenList &open);
    };

    class SymController;

    class UniformCostSearch : public UnidirectionalSearch {
        OpenList open_list;

        StepCostEstimation estimation; //Time/nodes estimated
        SymExpStatistics stats;

        // Returns the subset with h_value h
        BDD compute_heuristic(const BDD &from, int fVal, int hVal, bool store_eval);

        void computeEstimation(bool prepare);

        //////////////////////////////////////////////////////////////////////////////
    public:
        UniformCostSearch(SymController *eng, const SymParamsSearch &params);


        // Disable copy
        UniformCostSearch(const UniformCostSearch &) = delete;
        UniformCostSearch &operator=(const UniformCostSearch &) = delete;
        UniformCostSearch &operator=(UniformCostSearch &&) = delete;
        UniformCostSearch(UniformCostSearch &&) = default;
        virtual ~UniformCostSearch() = default;


        // Default initialization to the initial state and the goal from the manager
        void init(const std::shared_ptr<SymStateSpaceManager>& manager, bool fw);

        // Custom initialization to any initial state and goal
        void init(const std::shared_ptr<SymStateSpaceManager>& manager, bool fw, const BDD& init_bdd, std::shared_ptr<OppositeFrontier> frontier); // Init forward or backward search

        bool stepImage(utils::Duration maxTime, long maxNodes) override;

        void getPlan(const BDD &cut, int g, std::vector<OperatorID> &path) const override;

        bool finished() const override {
            assert(!open_list.empty() || closed->getGNotClosed() == std::numeric_limits<int>::max());
            return open_list.empty();
        }

        std::shared_ptr<ClosedList> getClosed() override {
            return closed;
        }

        virtual ADD getHeuristic() const;

        bool isSearchableWithNodes(int maxNodes) const override;

        // Pointer to the closed list. Used to set as heuristic of other explorations.
        inline ClosedList *getClosed() const {
            return closed.get();
        }

        inline std::shared_ptr<ClosedList> getClosedShared() const {
            return closed;
        }

        int getG() const override {
            return open_list.minG();
        }

        virtual int getF() const override {
            return open_list.minG();
        }

        utils::Duration nextStepTime() const override;

        long nextStepNodes() const override;

    private:
        friend std::ostream &operator<<(std::ostream &os, const UniformCostSearch &bdexp);
    };

}
#endif

