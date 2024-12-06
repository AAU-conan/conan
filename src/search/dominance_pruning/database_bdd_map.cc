#include "database_bdd_map.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

#include "../symbolic/sym_variables.h"
#include "../factored_transition_system/symbolic_state_mapping.h"
#include "../variable_ordering/variable_ordering_strategy.h"
using namespace symbolic;
namespace dominance {


    std::unique_ptr<DominanceDatabase> DatabaseBDDMapFactory::create(const std::shared_ptr<AbstractTask> & task,
                                                                     std::shared_ptr<FactoredDominanceRelation> dominance_relation,
                                                                      std::shared_ptr<fts::FactoredStateMapping> state_mapping) {

        auto vars = std::make_shared<SymVariables>(bdd_mgr, *variable_ordering_strategy, task);
        fts::FactoredSymbolicStateMapping sym_mapping (*state_mapping, vars);

      auto dominance_bdd = std::make_shared<DominanceRelationBDD>(*dominance_relation, sym_mapping, insert_dominated);
      if (insert_dominated) {
          return std::make_unique<DatabaseBDDMapDominated>(vars, dominance_bdd);
      } else {
          return std::make_unique<DatabaseBDDMapDominating>(vars, dominance_bdd);
        }
      }

    void DatabaseBDDMapDominated::insert(const ExplicitState &state, int g) {
        BDD res = dominance_relation_bdd->get_related_states(state);
        /*
                    if (remove_spurious_dominated_states) {
                        try {
                            res = mgr->filter_mutex(res, true, 1000000, true);
                            res = mgr->filter_mutex(res, false, 1000000, true);
                        } catch (BDDError) {
                            cout << "Disabling removal of spurious dominated states" << endl;
                            //If it is not possible, do not remove spurious states
                            remove_spurious_dominated_states = false;
                        }
                    }
        */
        if (vars->numStates(res) == 1) {
            //Small optimization: If we have a single state, not include it
            res = vars->zeroBDD();
        }

        if (!closed.count(g)) {
            closed[g] = res;
        } else {
            closed[g] += res;
        }
    }

    bool DatabaseBDDMapDominated::check(const ExplicitState &state, int g) const {
            auto sb = vars->getBinaryDescription(state);
            for (auto &entry: closed) {
                if (entry.first > g) break;
                if (!(entry.second.Eval(sb).IsZero())) {
                    return true;
                }
            }
        return false;
    }


    void DatabaseBDDMapDominating::insert(const ExplicitState &state, int g) {
        BDD res = vars->getStateBDD(state);;
        if (!closed.count(g)) {
            closed[g] = res;
        } else {
            closed[g] += res;
        }
    }

    bool DatabaseBDDMapDominating::check(const ExplicitState &state, int g) const {
            BDD simulatingBDD = dominance_relation_bdd->get_related_states(state);
            for (auto &entry: closed) {
                if (entry.first > g) break;
                if (!((entry.second * simulatingBDD).IsZero())) {
                    return true;
                }
            }

        return false;
    }


    class DominanceDatabaseBDDMapFeature
        : public plugins::TypedFeature<DominanceDatabaseFactory, DatabaseBDDMapFactory> {
    public:
        DominanceDatabaseBDDMapFeature() : TypedFeature("bdd_map") {
            document_title("All Previous Dominance Pruning");

            document_synopsis(
                "This pruning method implements a strategy where all previously generated states "
                "are compared against new states to see if the new state is dominated by any previous state.");

            add_option<std::shared_ptr<variable_ordering::VariableOrderingStrategy>>(
                    "variable_ordering",
                    "Strategy to decide the variable ordering",
                    "gamer()");

            BDDManager::add_options_to_feature(*this);
            add_option<bool>("insert_dominated", "Insert dominated or check dominating states", "true");
        }

        virtual std::shared_ptr<DatabaseBDDMapFactory> create_component(
            const plugins::Options &opts,
            const utils::Context &) const override {

            BDDManagerParameters mgr_params (opts);
            auto bdd_mgr = std::make_shared<BDDManager>(mgr_params);

            return plugins::make_shared_from_arg_tuples<DatabaseBDDMapFactory>(
                opts.get<bool>("insert_dominated"),
//                opts.get<bool>("remove_spurious_dominated_states"),
                bdd_mgr,
                opts.get < std::shared_ptr < variable_ordering::VariableOrderingStrategy>> ("variable_ordering"));
        }
    };

    static plugins::FeaturePlugin<DominanceDatabaseBDDMapFeature> _plugin;

}
