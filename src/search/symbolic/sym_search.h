#ifndef SYMBOLIC_SYM_SEARCH_H
#define SYMBOLIC_SYM_SEARCH_H

#include "sym_state_space_manager.h"
#include "bucket.h"

#include "../utils/logging.h"

#include <vector>
#include <map>
#include <memory>

namespace symbolic {
    class SymController;

    inline std::string dirname(bool fw) {
        return fw ? "fw" : "bw";
    }

    class SymParamsSearch {
    public:
        //By default max<int>. If lower, it allows for skip disjunction if the size of S is greater.
        long max_disj_nodes;

        //Parameters for sym_step_cost_estimation
        double min_estimation_time;         // Dont estimate if time is lower
        double penalty_time_estimation_sum; // violated_time = sum + time*mult
        double penalty_time_estimation_mult;
        long penalty_nodes_estimation_sum;// violated_nodes = sum + nodes*mult
        double penalty_nodes_estimation_mult;

        //Parameters to control isUseful() and isSearchable()
        utils::Duration maxStepTime;
        long maxStepNodes;

        //Allows to scale maxStepNodes with planning time (starts at 100*x
        // during 100s and then grows at a rate of 1x)
        // int maxStepNodesPerPlanningSecond, maxStepNodesMin, maxStepNodesTimeStartIncrement;

        // Parameters to decide the allotted time and nodes for a step
        // allotted = max(minAllotted, estimated*multAllotted)
        utils::Duration minAllottedTime, maxAllottedTime;
        long minAllottedNodes, maxAllottedNodes;
        double ratioAllottedTime, ratioAllottedNodes; // factor to multiply the estimation

        bool non_stop;

        mutable utils::LogProxy log;

        SymParamsSearch(const plugins::Options &opts);

        static void add_options_to_feature(plugins::Feature &parser, utils::Duration maxStepTime, long maxStepNodes);

        void print_options() const;

        inline utils::Duration getAllottedTime(double estimatedTime) const {
            return utils::Duration{std::min<double>(maxAllottedTime,
                                                    std::max<double>(estimatedTime * ratioAllottedTime,
                                                                     minAllottedTime))};
        }

        inline long getAllottedNodes(double estimatedNodes) const {
            return std::min(maxAllottedNodes,
                            std::max<long>(static_cast<long>(estimatedNodes * ratioAllottedNodes), minAllottedNodes));
        }

        void inheritParentParams(const SymParamsSearch &other) {
            maxStepTime = std::min(maxStepTime, other.maxStepTime);
            maxStepNodes = std::min(maxStepNodes, other.maxStepNodes);
        }

        int getMaxStepNodes() const;

        inline bool get_non_stop() const {
            return non_stop;
        }
    };

    class SymSearch {
    protected:
        //Attributes that characterize the search:
        std::shared_ptr<SymStateSpaceManager> mgr;           //Symbolic manager to perform bdd operations
        SymParamsSearch p;

        SymController *engine; //Access to the bound and notification of new solutions

    public:
        SymSearch(SymController *eng, const SymParamsSearch &params);

        SymStateSpaceManager *getStateSpace() {
            return mgr.get();
        }

        inline std::shared_ptr<SymStateSpaceManager> getStateSpaceShared() const {
            return mgr;
        }

        inline bool isSearchable() const {
            return isSearchableWithNodes(p.getMaxStepNodes());
        }

        bool step() {
            return stepImage(p.getAllottedTime(nextStepTime()),
                             p.getAllottedNodes(nextStepNodes()));
        }


        utils::LogProxy &get_log() const {
            return p.log;
        }

        virtual bool stepImage(utils::Duration maxTime, long maxNodes) = 0;

        virtual int getF() const = 0;

        virtual bool finished() const = 0;

        virtual utils::Duration nextStepTime() const = 0;

        virtual long nextStepNodes() const = 0;

        virtual void statistics() const = 0;

        virtual bool isSearchableWithNodes(int maxNodes) const = 0;

        virtual ~SymSearch() = default;
    };
}
#endif
