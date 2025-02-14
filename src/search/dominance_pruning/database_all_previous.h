#ifndef DOMINANCE_DATABASE_ALL_PREVIOUS_H
#define DOMINANCE_DATABASE_ALL_PREVIOUS_H

#include "dominance_database.h"

namespace dominance {
class DatabaseAllPrevious : public DominanceDatabase {
    std::vector<ExplicitState> previous_transformed_states;

    std::shared_ptr<StateDominanceRelation> dominance_relation;
    //std::shared_ptr<fts::FactoredStateMapping> state_mapping;

public:
    DatabaseAllPrevious(std::shared_ptr<StateDominanceRelation> dominance_relation)
        : dominance_relation(dominance_relation){//, state_mapping(state_mapping) {
    }
    virtual ~DatabaseAllPrevious() = default;

    //Methods to keep dominated states in explicit search
    //Check: returns true if a better or equal state is known
    bool check(const ExplicitState &state, int g) const override;
    //Insert: inserts a state into the set of known states
    void insert(const ExplicitState &transformed_state, int g) override;


};

    class DatabaseAllPreviousFactory : public DominanceDatabaseFactory {
    public:
        DatabaseAllPreviousFactory() : DominanceDatabaseFactory() {
        }

        virtual std::unique_ptr<DominanceDatabase> create(const std::shared_ptr<AbstractTask> &,
                                                          std::shared_ptr<StateDominanceRelation> dominance_relation,
                                                          std::shared_ptr<fts::FactoredStateMapping> ) override
        {
            return std::make_unique<DatabaseAllPrevious>(dominance_relation);
        }
    };

}

#endif