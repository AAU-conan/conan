#include "bdd_manager.h"

#include <string>
#include "../plugins/options.h"
#include "../plugins/plugin.h"
#include <cassert>

using namespace std;
using plugins::Options;

namespace symbolic {
    //Initialize manager
    void exceptionError(string /*message*/) {
        //cout << message << endl;
        throw BDDError();
    }

    void exitOutOfMemory(size_t) {
        cerr << "Memory exceeded within BDD operation" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_OUT_OF_MEMORY);
    }

    bool bdd_intersection_empty(const BDD &bdd1, const BDD &bdd2) {
        return bdd1.Leq(!bdd2); //If bdd1 implies !bdd2 then their intersection is empty
    }

    BDDManager::BDDManager(const Options &opts) :
            cudd_init_nodes(opts.get<int>("cudd_init_nodes")),
            cudd_init_cache_size(opts.get<int>("cudd_init_cache_size")),
            cudd_init_available_memory(opts.get<int>("cudd_init_available_memory")) {
    }

    void BDDManager::init(int num_bdd_vars) {
        assert (!_manager); //For now, we should only initialize the manager once

        if (utils::g_log.is_at_least_verbose()) {
            utils::g_log << "Initialize Symbolic Manager(" << num_bdd_vars << ", "
                         << cudd_init_nodes / num_bdd_vars << ", "
                         << cudd_init_cache_size << ", "
                         << cudd_init_available_memory << ")" << endl;
        }
        _manager = make_unique<Cudd>(num_bdd_vars, 0,
                                     cudd_init_nodes / num_bdd_vars, cudd_init_cache_size, cudd_init_available_memory);

        _manager->setHandler(exceptionError);
        _manager->setTimeoutHandler(exceptionError);
        _manager->setNodesExceededHandler(exceptionError);
        _manager->RegisterOutOfMemoryCallback(exitOutOfMemory);
    }

    void BDDManager::add_options_to_feature(plugins::Feature &feature) {
        feature.add_option<int>("cudd_init_nodes", "Initial number of nodes in the cudd manager.", "16000000");
        feature.add_option<int>("cudd_init_cache_size", "Initial number of cache entries in the cudd manager.",
                                "16000000");
        feature.add_option<int>("cudd_init_available_memory", "Total available memory for the cudd manager.", "0");
    }

    ADD BDDManager::getADD(const std::map<int, BDD> &heur) {
        ADD total = mgr()->plusInfinity();

        for (const auto &entry: heur) {
            ADD x_complement = (!entry.second).Add()*mgr()->plusInfinity();
            ADD x = entry.second.Add() * getADD(entry.first);
            total = total.Minimum(x + x_complement);
        }

        return total;
    }

    void BDDManager::print_options() const {
        cout << "CUDD Init: nodes=" << cudd_init_nodes <<
             " cache=" << cudd_init_cache_size <<
             " max_memory=" << cudd_init_available_memory << endl;
    }

    bool BDDManager::perform_operation_with_time_limit(utils::Duration maxTime, const std::function<void()> &function) {
        if (!maxTime.is_infinity()) {
            setTimeLimit(static_cast<unsigned long>(maxTime * 1000));
            //Compute image, storing the result on Simg
            try {
                function();
                unsetTimeLimit();
                return true;
            } catch (BDDError e) {
                unsetTimeLimit();
                return false;
            }
        } else {
            try {
                function();
                return true;
            } catch (BDDError e) {
                return false;
            }
        }
    }

    std::optional<BDD>
    BDDManager::compute_bdd_with_time_limit(utils::Duration maxTime, const std::function<BDD()> &function) {
        if (!maxTime.is_infinity()) {
            setTimeLimit(static_cast<unsigned long>(maxTime * 1000));
            //Compute image, storing the result on Simg
            try {
                BDD result = function();
                unsetTimeLimit();
                return result;
            } catch (BDDError e) {
                unsetTimeLimit();
                return std::nullopt;
            }
        } else {
            try {
                BDD result = function();
                return result;
            } catch (BDDError e) {
                return std::nullopt;
            }
        }
    }
}
