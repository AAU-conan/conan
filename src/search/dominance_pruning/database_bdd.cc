#include "database_bdd.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

#include "../symbolic/sym_variables.h"
#include "../factored_transition_system/symbolic_state_mapping.h"
#include "../variable_ordering/variable_ordering_strategy.h"
#include "../dominance/dominance_relation_bdd.h"
using namespace symbolic;
namespace dominance {

    DatabaseBDDDominated::DatabaseBDDDominated(std::shared_ptr<symbolic::SymVariables> vars,
                            std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd) : DominanceDatabase(),
        vars(vars),
        dominance_relation_bdd(dominance_relation_bdd), closed(vars->zeroBDD()) {
    }

    DatabaseBDDDominating::DatabaseBDDDominating(std::shared_ptr<symbolic::SymVariables> vars,
                             std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd) : DominanceDatabase(),
        vars(vars), dominance_relation_bdd(dominance_relation_bdd), closed(vars->zeroBDD()) {
    }



    std::unique_ptr<DominanceDatabase> DatabaseBDDFactory::create(const std::shared_ptr<AbstractTask> & task,
                                                                     std::shared_ptr<StateDominanceRelation> dominance_relation,
                                                                      std::shared_ptr<fts::FactoredStateMapping> state_mapping) {

        auto vars = std::make_shared<SymVariables>(bdd_mgr, *variable_ordering_strategy, task);
        fts::FactoredSymbolicStateMapping sym_mapping (*state_mapping, vars);

      auto dominance_bdd = std::make_shared<DominanceRelationBDD>(*dominance_relation, sym_mapping, insert_dominated);
      if (insert_dominated) {
          return std::make_unique<DatabaseBDDDominated>(vars, dominance_bdd);
      } else {
          return std::make_unique<DatabaseBDDDominating>(vars, dominance_bdd);
        }
      }

    void DatabaseBDDDominated::insert(const ExplicitState &state, int ) {
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

        closed += res;
    }

    bool DatabaseBDDDominated::check(const ExplicitState &state, int ) const {
            auto sb = vars->getBinaryDescription(state);
            return !(closed.Eval(sb).IsZero());
    }


    void DatabaseBDDDominating::insert(const ExplicitState &state, int ) {
        closed += vars->getStateBDD(state);
    }

    bool DatabaseBDDDominating::check(const ExplicitState &state, int ) const {
        BDD simulatingBDD = dominance_relation_bdd->get_related_states(state);
        return !(closed.Intersect(simulatingBDD).IsZero());
    }



    class DominanceDatabaseBDDFeature
        : public plugins::TypedFeature<DominanceDatabaseFactory, DatabaseBDDFactory> {
    public:
        DominanceDatabaseBDDFeature() : TypedFeature("bdd") {
            add_option<std::shared_ptr<variable_ordering::VariableOrderingStrategy>>(
                    "variable_ordering",
                    "Strategy to decide the variable ordering",
                    "gamer()");

            BDDManager::add_options_to_feature(*this);
            add_option<bool>("insert_dominated", "Insert dominated or check dominating states", "true");
        }

        virtual std::shared_ptr<DatabaseBDDFactory> create_component(const plugins::Options &opts) const override {

            BDDManagerParameters mgr_params (opts);
            auto bdd_mgr = std::make_shared<BDDManager>(mgr_params);

            return plugins::make_shared_from_arg_tuples<DatabaseBDDFactory>(
                opts.get<bool>("insert_dominated"),
//                opts.get<bool>("remove_spurious_dominated_states"),
                bdd_mgr,
                opts.get < std::shared_ptr < variable_ordering::VariableOrderingStrategy>> ("variable_ordering"));
        }
    };

    static plugins::FeaturePlugin<DominanceDatabaseBDDFeature> _plugin;

}
