#ifndef DOMINANCE_DATABASE_PREVIOUS_LOWER_G_H
#define DOMINANCE_DATABASE_PREVIOUS_LOWER_G_H

#include <map>


#include "dominance_database.h"

namespace dominance {

    class DatabasePreviousLowerG : public DominanceDatabase {
        // key: g_value; value: vector<vector<int>> with the transformed states with that g_value
        std::map<int, std::vector<std::vector<int>>> previous_states_sorted;

        std::shared_ptr<StateDominanceRelation> dominance_relation;
        //std::shared_ptr<fts::FactoredStateMapping> state_mapping;

    public:
        DatabasePreviousLowerG(std::shared_ptr<StateDominanceRelation> dominance_relation)
                                //  std::shared_ptr<fts::FactoredStateMapping> state_mapping)
          : dominance_relation(dominance_relation) {//, state_mapping(state_mapping) {
        }
        virtual ~DatabasePreviousLowerG() = default;

        //Methods to keep dominated states in explicit search
        //Check: returns true if a better or equal state is known
        bool check(const ExplicitState &succ_transformed, int g_value) const override;
        //Insert: inserts a state into the set of known states
        void insert(const ExplicitState &transformed_state, int g_value) override;
    };


    class DatabasePreviousLowerGFactory : public DominanceDatabaseFactory {
        public:
        DatabasePreviousLowerGFactory() : DominanceDatabaseFactory() {
        }

        virtual std::unique_ptr<DominanceDatabase> create(const std::shared_ptr<AbstractTask> &,
                                                          std::shared_ptr<StateDominanceRelation> dominance_relation,
                                                          std::shared_ptr<fts::FactoredStateMapping> ) override
        {
          return std::make_unique<DatabasePreviousLowerG>(dominance_relation);
          }
    };


}


#endif