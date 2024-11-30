#ifndef DOMINANCE_DATABASE_ALL_PREVIOUS_H
#define DOMINANCE_DATABASE_ALL_PREVIOUS_H

#include "dominance_database.h"

namespace dominance {
class DatabaseTest : public DominanceDatabase {

  std::vector<std::unique_ptr<DominanceDatabase>> dbs;
public:
    DatabaseTest(std::vector<std::unique_ptr<DominanceDatabase>> && dbs)
        : dbs(std::move(dbs)) {
    }
    virtual ~DatabaseTest() = default;

    //Methods to keep dominated states in explicit search
    //Check: returns true if a better or equal state is known
    bool check(const ExplicitState &state, int g) const override;
    //Insert: inserts a state into the set of known states
    void insert(const ExplicitState &transformed_state, int g) override;


};

    class DatabaseTestFactory : public DominanceDatabaseFactory {
        std::vector<std::shared_ptr<DominanceDatabaseFactory>> db_factories;

    public:
        DatabaseTestFactory(        std::vector<std::shared_ptr<DominanceDatabaseFactory>> db_factories) : DominanceDatabaseFactory(), db_factories(std::move(db_factories)) {
        }

        virtual std::unique_ptr<DominanceDatabase> create(const std::shared_ptr<AbstractTask> & task,
                                                          std::shared_ptr<FactoredDominanceRelation> dominance_relation,
                                                          std::shared_ptr<fts::FactoredStateMapping> mapping) override
        {
            std::vector<std::unique_ptr<DominanceDatabase>> dbs;

            for (const auto &db_factory : db_factories) {
                dbs.push_back(db_factory->create(task, dominance_relation, mapping));
            }
            return std::make_unique<DatabaseTest>(std::move(dbs));
        }
    };

}

#endif