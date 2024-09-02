#ifndef SYMBOLIC_BDD_MANAGER_H
#define SYMBOLIC_BDD_MANAGER_H

#include "cuddObj.hh"

#include "../utils/timer.h"

#include <map>
#include <memory>
#include <functional>
#include <optional>

namespace plugins {
    class Options;

    class Feature;
}
namespace symbolic {

    struct BDDError {
    };

    extern void exceptionError(std::string message);

    extern void exitOutOfMemory(size_t memory);

    class BDDManager {
        //Parameters to initialize the CUDD manager
        const long cudd_init_nodes; //Number of initial nodes
        const long cudd_init_cache_size; //Initial cache size
        const long cudd_init_available_memory; //Maximum available memory (bytes)

        std::unique_ptr<Cudd> _manager; //_manager associated with this symbolic search

        void setTimeLimit(unsigned long maxTime) {
            _manager->SetTimeLimit(maxTime);
            _manager->ResetStartTime();
        }

        void unsetTimeLimit() {
            _manager->UnsetTimeLimit();
        }

    public:
        BDDManager(const plugins::Options &opts);

        void init(int num_bdd_vars);


        // Functions to add a global time limit to all BDD operations within function.
        // If time limit is exceeded, it returns false.
        bool perform_operation_with_time_limit(utils::Duration maxTime, const std::function<void()> &function);

        // If time limit is exceeded, no BDD is returned.
        std::optional<BDD> compute_bdd_with_time_limit(utils::Duration maxTime, const std::function<BDD()> &function);

        long totalNodes() const {
            return _manager->ReadNodeCount();
        }

        unsigned long totalMemory() const {
            return _manager->ReadMemoryInUse();
        }

        double totalMemoryGB() const {
            return _manager->ReadMemoryInUse() / (1024 * 1024 * 1024);
        }

        BDD zeroBDD() const {
            return _manager->bddZero();
        }

        BDD oneBDD() const {
            return _manager->bddOne();
        }

        Cudd *mgr() const {
            return _manager.get();
        }

        BDD bddVar(int v) const {
            return _manager->bddVar(v);
        }

        int usedNodes() const {
            return _manager->ReadSize();
        }

        ADD getADD(int value) {
            return _manager->constant(value);
        }

        ADD getADD(const std::map<int, BDD> &heur);

        static void add_options_to_feature(plugins::Feature &feature);

        void print_options() const;
    };

    bool bdd_intersection_empty (const BDD & bdd1, const BDD & bdd2);

}

#endif
