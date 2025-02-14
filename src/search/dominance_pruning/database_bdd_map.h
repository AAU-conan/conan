#ifndef DOMINANCE_DATABASE_BDD_MAP_H
#define DOMINANCE_DATABASE_BDD_MAP_H


#include <map>
#include "dominance_database.h"
#include "../dominance/dominance_relation_bdd.h"

namespace symbolic {
    class SymVariables;
}

namespace variable_ordering {
    class VariableOrderingStrategy;
}

namespace dominance {
    class DatabaseBDDMapDominated : public DominanceDatabase {
        std::shared_ptr<symbolic::SymVariables> vars;
        std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd;

        std::map<int, BDD> closed;

    public:
        DatabaseBDDMapDominated(std::shared_ptr<symbolic::SymVariables> vars,
                                std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd) : DominanceDatabase(),
            vars(vars),
            dominance_relation_bdd(dominance_relation_bdd) {
        }

        virtual ~DatabaseBDDMapDominated() = default;

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };

    class DatabaseBDDMapDominating : public DominanceDatabase {
        std::shared_ptr<symbolic::SymVariables> vars;
        std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd;

        std::map<int, BDD> closed;

    public:
        DatabaseBDDMapDominating(std::shared_ptr<symbolic::SymVariables> vars,
                                 std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd) : DominanceDatabase(),
            vars(vars), dominance_relation_bdd(dominance_relation_bdd) {
        }

        virtual ~DatabaseBDDMapDominating() = default;

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };


    class DatabaseBDDMapFactory : public DominanceDatabaseFactory {
        const bool insert_dominated;
        std::shared_ptr<symbolic::BDDManager> bdd_mgr;
        std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy;

    public:
        DatabaseBDDMapFactory(bool insert_dominated, std::shared_ptr<symbolic::BDDManager> bdd_mgr,
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
