#ifndef DOMINANCE_DATABASE_BDD_MAP_DISJ_H
#define DOMINANCE_DATABASE_BDD_MAP_DISJ_H


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
    class DatabaseBDDMapDisjDominated : public DominanceDatabase {
        const int max_bdd_size;

        std::shared_ptr<symbolic::SymVariables> vars;
        std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd;


        std::map<int, std::vector<BDD>> closed;

    public:
        DatabaseBDDMapDisjDominated(int max_bdd_size, std::shared_ptr<symbolic::SymVariables> vars,
                                std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd) : DominanceDatabase(), max_bdd_size(max_bdd_size),
            vars(vars),
            dominance_relation_bdd(dominance_relation_bdd) {
        }

        virtual ~DatabaseBDDMapDisjDominated() = default;

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };

    class DatabaseBDDMapDisjDominating : public DominanceDatabase {
        const int max_bdd_size;

        std::shared_ptr<symbolic::SymVariables> vars;
        std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd;

        std::map<int, std::vector<BDD>> closed;

    public:
        DatabaseBDDMapDisjDominating(int max_bdd_size, std::shared_ptr<symbolic::SymVariables> vars,
                                 std::shared_ptr<DominanceRelationBDD> dominance_relation_bdd) : DominanceDatabase(), max_bdd_size(max_bdd_size),
            vars(vars), dominance_relation_bdd(dominance_relation_bdd) {
        }

        virtual ~DatabaseBDDMapDisjDominating() = default;

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };


    class DatabaseBDDMapDisjFactory : public DominanceDatabaseFactory {
        const bool insert_dominated;
        const int max_bdd_size;
        std::shared_ptr<symbolic::BDDManager> bdd_mgr;
        std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy;

    public:
        DatabaseBDDMapDisjFactory(bool insert_dominated, int max_bdd_size, std::shared_ptr<symbolic::BDDManager> bdd_mgr,
                              std::shared_ptr<variable_ordering::VariableOrderingStrategy>
                              variable_ordering_strategy) : insert_dominated(insert_dominated), max_bdd_size(max_bdd_size),
                                                            bdd_mgr(bdd_mgr),
                                                            variable_ordering_strategy(variable_ordering_strategy) {
        }

        virtual std::unique_ptr<DominanceDatabase> create(const std::shared_ptr<AbstractTask> &task,
                                                          std::shared_ptr<FactoredDominanceRelation> dominance_relation,
                                                          std::shared_ptr<fts::FactoredStateMapping> state_mapping) override;
    };
}

#endif
