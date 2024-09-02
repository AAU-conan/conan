#ifndef VARIABLE_ORDERING_VARIABLE_ORDERING_GAMER_H
#define VARIABLE_ORDERING_VARIABLE_ORDERING_GAMER_H

#include "variable_ordering_strategy.h"
#include <vector>
#include <memory>

namespace plugins {
    class Options;
}

namespace utils {
    class RandomNumberGenerator;
}
namespace variable_ordering {

    class InfluenceGraph {
        std::vector<std::vector<int>> influence_graph;

        int influence(int v1, int v2) const {
            return influence_graph[v1] [v2];
        }
    public:
        InfluenceGraph(int n);

        void set_influence(int v1, int v2, int val = 1) {
            influence_graph[v1][v2] = val;
            influence_graph[v2][v1] = val;
        }

        double compute_function(const std::vector <int> &order) const;

        double compute_function_incremental_swap(const std::vector <int> &order,
                                                 double totalDistance, int swapIndex1, int swapIndex2) const;
    };


    class GamerVariableOrdering : public VariableOrderingStrategy {
    private:
        std::shared_ptr<utils::RandomNumberGenerator> rng;
        const int runs;
        const int iterations_per_run;

        double optimize_variable_ordering_gamer(const InfluenceGraph & ig, int iterations, std::vector <int> &order) const;

        /*TODO: void optimize_variable_ordering_gamer(const InfluenceGraph & ig, std::vector <int> &order,
                                              std::vector <int> &partition_begin,
                                              std::vector <int> &partition_sizes,
                                              int iterations = 50000) const;*/

    public:
        explicit GamerVariableOrdering(int runs, int iterations_per_run, int random_seed);
        virtual ~GamerVariableOrdering() = default;
        std::vector<int> compute_variable_ordering(const AbstractTask & task) const override;
        virtual void print_options() const override;

    };
}
#endif
