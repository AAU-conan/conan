#include "variable_ordering_gamer.h"

#include "../task_proxy.h"
#include "../task_utils/causal_graph.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../plugins/plugin.h"
#include "../utils/logging.h"
#include "../utils/markup.h"
#include "../utils/rng_options.h"

using namespace std;
using plugins::Options;
using causal_graph::CausalGraph;

namespace variable_ordering {
    GamerVariableOrdering::GamerVariableOrdering(int runs, int iterations_per_run, int random_seed) :
            rng(utils::get_rng(random_seed)), runs(runs), iterations_per_run(iterations_per_run) {
    }

    void GamerVariableOrdering::print_options() const {
        cout << "Gamer variable ordering, runs " << runs << ", iterations per run: " << iterations_per_run << endl;
    }
    //Returns a optimized variable ordering that reorders the variables
    //according to the standard causal graph criterion

    vector<int> GamerVariableOrdering::compute_variable_ordering(const AbstractTask &task) const {
        TaskProxy task_proxy(task);

        const CausalGraph &cg = task_proxy.get_causal_graph();

        vector<int> var_order;
        if (var_order.empty()) {
            for (int v = 0; v < task.get_num_variables(); v++) {
                var_order.push_back(v);
            }
        }

        InfluenceGraph ig_partitions(task.get_num_variables());
        for (int v = 0; v < task.get_num_variables(); v++) {
            for (int v2: cg.get_successors(v)) {
                if ((int) v != v2) {
                    ig_partitions.set_influence(v, v2);
                }
            }
        }

        double value_optimization_function = optimize_variable_ordering_gamer(ig_partitions, iterations_per_run,
                                                                              var_order);

        //DEBUG_MSG(cout << "Value: " << value_optimization_function << endl;);

        for (int counter = 0; counter < runs; counter++) {
            vector<int> new_order = var_order;
            rng->shuffle(new_order);
            double new_value = optimize_variable_ordering_gamer(ig_partitions, iterations_per_run, var_order);

            if (new_value < value_optimization_function) {
                value_optimization_function = new_value;
                var_order.swap(new_order);
                //DEBUG_MSG(cout << "New value: " << value_optimization_function << endl;);
            }
        }

        // cout << "Var ordering: ";
        // for(int v : var_order) cout << v << " ";
        // cout  << endl;
        return var_order;
    }

    double InfluenceGraph::compute_function_incremental_swap(const vector<int> &order,
                                                             double totalDistance, int swapIndex1,
                                                             int swapIndex2) const {
        //Compute the new value of the optimization function
        for (int i = 0; i < int(order.size()); i++) {
            if ((int) i == swapIndex1 || (int) i == swapIndex2)
                continue;

            if (influence(order[i], order[swapIndex1]))
                totalDistance += (-(i - swapIndex1) * (i - swapIndex1)
                                  + (i - swapIndex2) * (i - swapIndex2));

            if (influence(order[i], order[swapIndex2]))
                totalDistance += (-(i - swapIndex2) * (i - swapIndex2)
                                  + (i - swapIndex1) * (i - swapIndex1));
        }

        return totalDistance;
    }

    double GamerVariableOrdering::optimize_variable_ordering_gamer(const InfluenceGraph &ig, int iterations,
                                                                   vector<int> &order) const {
        double totalDistance = ig.compute_function(order);

        double oldTotalDistance = totalDistance;
        //Repeat iterations times
        for (int counter = 0; counter < iterations; counter++) {
            //Swap variable
            int swapIndex1 = rng->random(order.size());
            int swapIndex2 = rng->random(order.size());
            if (swapIndex1 == swapIndex2)
                continue;

            totalDistance = ig.compute_function_incremental_swap(order, totalDistance, swapIndex1, swapIndex2);


            //Apply the swap if it is worthy
            if (totalDistance < oldTotalDistance) {
                int tmp = order[swapIndex1];
                order[swapIndex1] = order[swapIndex2];
                order[swapIndex2] = tmp;
                oldTotalDistance = totalDistance;

                /*if(totalDistance != compute_function(order)){
                  cerr << "Error computing total distance: " << totalDistance << " " << compute_function(order) << endl;
                  exit(-1);
                }else{
                  cout << "Bien: " << totalDistance << endl;
                }*/
            } else {
                totalDistance = oldTotalDistance;
            }
        }
//  cout << "Total distance: " << totalDistance << endl;
        return totalDistance;
    }


    double InfluenceGraph::compute_function(const std::vector<int> &order) const {
        double totalDistance = 0;
        for (size_t i = 0; i < order.size() - 1; i++) {
            for (size_t j = i + 1; j < order.size(); j++) {
                if (influence(order[i], order[j])) {
                    totalDistance += (j - i) * (j - i);
                }
            }
        }
        return totalDistance;
    }


    InfluenceGraph::InfluenceGraph(int num) {
        influence_graph.resize(num);
        for (auto &i: influence_graph) {
            i.resize(num, 0);
        }
    }

/*
    void InfluenceGraph::optimize_variable_ordering_gamer(vector <int> &order,
                                                          vector <int> &partition_begin,
                                                          vector <int> &partition_sizes,
                                                          int iterations) const {
        double totalDistance = compute_function(order);

        double oldTotalDistance = totalDistance;
        //Repeat iterations times
        for (int counter = 0; counter < iterations; counter++) {
            //Swap variable
            int partition = (*g_rng())(partition_begin.size());
            if (partition_sizes[partition] <= 1)
                continue;
            int swapIndex1 = partition_begin[partition] + (*g_rng())(partition_sizes[partition]);
            int swapIndex2 = partition_begin[partition] + (*g_rng())(partition_sizes[partition]);
            if (swapIndex1 == swapIndex2)
                continue;

            //Compute the new value of the optimization function
            for (int i = 0; i < int(order.size()); i++) {
                if ((int)i == swapIndex1 || (int)i == swapIndex2)
                    continue;

                if (influence(order[i], order[swapIndex1]))
                    totalDistance += (-(i - swapIndex1) * (i - swapIndex1)
                                      + (i - swapIndex2) * (i - swapIndex2));

                if (influence(order[i], order[swapIndex2]))
                    totalDistance += (-(i - swapIndex2) * (i - swapIndex2)
                                      + (i - swapIndex1) * (i - swapIndex1));
            }

            //Apply the swap if it is worthy
            if (totalDistance < oldTotalDistance) {
                int tmp = order[swapIndex1];
                order[swapIndex1] = order[swapIndex2];
                order[swapIndex2] = tmp;
                oldTotalDistance = totalDistance;

                if(totalDistance != compute_function(order)){
                  cerr << "Error computing total distance: " << totalDistance << " " << compute_function(order) << endl;
                  exit(-1);
                }else{
                  cout << "Bien: " << totalDistance << endl;
                }
            } else {
                totalDistance = oldTotalDistance;
            }
        }
//  cout << "Total distance: " << totalDistance << endl;
    }
*/



    class GamerVariableOrderingFeature : public plugins::TypedFeature<VariableOrderingStrategy, GamerVariableOrdering> {
    public:
        GamerVariableOrderingFeature() : TypedFeature("gamer") {
            document_title("variable ordering strategy");
            document_synopsis(
                    "This variable ordering strategy implements the algorithm described in"
                    " the paper:" + utils::format_conference_reference(
                            {"Peter Kissmann", "Stefan Edelkamp"},
                            "Improving Cost-Optimal Domain-Independent Symbolic Planning",
                            "https://ojs.aaai.org/index.php/AAAI/article/view/8009",
                            "Proceedings of the Twenty-Fifth {AAAI} Conference on Artificial Intelligence (AAAI 2011)",
                            "992-997",
                            "AAAI Press",
                            "2011"));

            utils::add_rng_options_to_feature(*this);

            add_option<int>(
                    "runs",
                    "number of times the procedure is triggered",
                    "20",
                    plugins::Bounds("1", "infinity"));

            add_option<int>(
                    "iterations_per_run",
                    "number of iterations the optimization takes per run",
                    "50000",
                    plugins::Bounds("1", "infinity"));
        }

        virtual shared_ptr<GamerVariableOrdering> create_component(const plugins::Options &opts) const override {
            return plugins::make_shared_from_arg_tuples<GamerVariableOrdering>(
                    opts.get<int>("runs"),
                    opts.get<int>("iterations_per_run"),
                    utils::get_rng_arguments_from_options(opts));
        }

    };

    static plugins::FeaturePlugin<GamerVariableOrderingFeature> _plugin;
}