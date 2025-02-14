#ifndef DOMINANCE_DATABASE_BDD_H
#define DOMINANCE_DATABASE_BDD_H

#include <map>
#include "dominance_database.h"
#include "../symbolic/bdd_manager.h"

namespace symbolic {
    class SymVariables;
}

namespace variable_ordering {
    class VariableOrderingStrategy;
}

namespace dominance {
    class StateDominanceRelation;
    class DominanceRelationBDD;

    class DatabaseBDDDominated : public DominanceDatabase {
        std::shared_ptr<symbolic::SymVariables> vars;
        std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd;
        BDD closed;
    public:
        DatabaseBDDDominated(std::shared_ptr<symbolic::SymVariables> vars,
                             std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd);
        virtual ~DatabaseBDDDominated() = default;

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };

    class DatabaseBDDDominating : public DominanceDatabase {
        std::shared_ptr<symbolic::SymVariables> vars;
        std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd;

       BDD closed;

    public:
        DatabaseBDDDominating(std::shared_ptr<symbolic::SymVariables> vars,
                                 std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd);
        virtual ~DatabaseBDDDominating() = default;

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };


    class DatabaseBDDFactory : public DominanceDatabaseFactory {
        const bool insert_dominated;
        std::shared_ptr<symbolic::BDDManager> bdd_mgr;
        std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy;

    public:
        DatabaseBDDFactory(bool insert_dominated, std::shared_ptr<symbolic::BDDManager> bdd_mgr,
                              std::shared_ptr<variable_ordering::VariableOrderingStrategy>
                              variable_ordering_strategy) : insert_dominated(insert_dominated),
                                                            bdd_mgr(bdd_mgr),
                                                            variable_ordering_strategy(variable_ordering_strategy) {
        }

        virtual std::unique_ptr<DominanceDatabase> create(const std::shared_ptr<AbstractTask> &task,
                                                          std::shared_ptr<StateDominanceRelation> dominance_relation,
                                                          std::shared_ptr<fts::FactoredStateMapping> state_mapping) override;
    };
}

#endif
