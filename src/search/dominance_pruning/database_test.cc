#include "database_test.h"

#include "../plugins/plugin.h"

namespace dominance {
    void DatabaseTest::insert(const std::vector<int>& transformed_state, int g) {
        std::cout << "Inserted state " << transformed_state << " with g=" << g << std::endl;
        for (auto& db : dbs) {
            db->insert(transformed_state, g);
        }
    }

    bool DatabaseTest::check(const ExplicitState &succ_transformed, int g_val) const {
      bool result = dbs[0]->check(succ_transformed, g_val);
        for (size_t i = 1; i < dbs.size(); ++i) {
            if (dbs[i]->check(succ_transformed, g_val) != result) {
                std::cout << "Databases disagree on state " << succ_transformed << " with g=" << g_val << std::endl;
                utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
            }else {
                std::cout << "Databases agree on state " << succ_transformed << " with g=" << g_val << std::endl;
            }
        }
        return result;
    }


    class DatabaseTestFactoryFeature
        : public plugins::TypedFeature<DominanceDatabaseFactory, DatabaseTestFactory> {
    public:
        DatabaseTestFactoryFeature() : TypedFeature("test") {
            document_title("Test");

            document_synopsis("Compares the result of databases.");

            add_list_option<std::shared_ptr<DominanceDatabaseFactory>>("dbs", "at least one database");
        }

        virtual std::shared_ptr<DatabaseTestFactory> create_component(const plugins::Options & opts) const override {
            return std::make_shared<DatabaseTestFactory>(opts.get_list<std::shared_ptr<DominanceDatabaseFactory>>("dbs"));
        }
    };

    static plugins::FeaturePlugin<DatabaseTestFactoryFeature> _plugin;


}
