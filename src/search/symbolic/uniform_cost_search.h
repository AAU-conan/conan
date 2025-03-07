#ifndef SYMBOLIC_UNIFORM_COST_SEARCH_H
#define SYMBOLIC_UNIFORM_COST_SEARCH_H

#include "sym_state_space_manager.h"
#include "closed_list.h"

#include "../algorithms/step_time_estimation.h"
#include "../algorithms/priority_queues.h"

#include <vector>
#include <map>
#include <memory>

#include "sym_search.h"

namespace symbolic {
    class OppositeFrontier;

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

        int old_g() const {
            return state_cost;
        }
        int new_g() const;

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
        OpenList(const SymStateSpaceManager &mgr);

        bool empty() const;

        StepImage pop();

        int get_min_new_g() const;
        int get_min_old_g() const;

        void insert_zero(int state_cost, BDD bdd);
        void insert_cost(int state_cost, BDD bdd);

        void insert(const StepImage & step);

        const StepImage & nextStep() const;

        friend std::ostream &operator<<(std::ostream &os, const OpenList &open);
    };

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

    class SymController;

    class UniformCostSearch : public SymSearch {
        bool fw; //Direction of the search. true=forward, false=backward

        OpenList open_list;
        std::shared_ptr<ClosedList> closed;

        std::shared_ptr<OppositeFrontier> perfectHeuristic;

        StepCostEstimation estimation;
        SymExpStatistics stats;

        // Returns the subset with h_value h
        BDD compute_heuristic(const BDD &from, int fVal, int hVal, bool store_eval);

        void computeEstimation(bool prepare);
    public:
        UniformCostSearch(const SymParamsSearch &params, const std::shared_ptr<SymStateSpaceManager>& manager, bool fw,
            std::shared_ptr<SymSolutionLowerBound> solution);

        UniformCostSearch(const SymParamsSearch &params, const std::shared_ptr<SymStateSpaceManager>& manager, bool fw,
            const BDD& init_bdd, std::shared_ptr<ClosedList> closed, std::shared_ptr<OppositeFrontier> frontier,
            std::shared_ptr<SymSolutionLowerBound> solution);

        UniformCostSearch(const UniformCostSearch &) = delete;
        UniformCostSearch &operator=(const UniformCostSearch &) = delete;
        UniformCostSearch &operator=(UniformCostSearch &&) = delete;
        UniformCostSearch(UniformCostSearch &&) = default;
        virtual ~UniformCostSearch() = default;

        bool stepImage(utils::Duration maxTime, long maxNodes) override;

        void getPlan(const BDD &cut, int g, std::vector<OperatorID> &path) const;

        bool finished() const override;

        const std::shared_ptr<ClosedList> & getClosed() const {
            return closed;
        }

        bool isSearchableWithNodes(int maxNodes) const override;

        inline std::shared_ptr<ClosedList> getClosedShared() const {
            return closed;
        }

        int getG() const override {
            // TODO: revise how this is computed. one could strenghten them based on having a separate g-value per operator cost
            return open_list.get_min_old_g();
        }

        virtual int getF() const override {
            return open_list.get_min_new_g();
        }

        utils::Duration nextStepTime() const override;

        long nextStepNodes() const override;


        void statistics() const override;

        bool isFW() const {
            return fw;
        }
    private:
        friend std::ostream &operator<<(std::ostream &os, const UniformCostSearch &bdexp);
    };

}
#endif

