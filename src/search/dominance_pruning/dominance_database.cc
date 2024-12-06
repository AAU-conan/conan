#include "dominance_database.h"

#include "../plugins/plugin.h"

namespace dominance {
    static class DominanceDatabaseFactoryCategoryPlugin : public plugins::TypedCategoryPlugin<DominanceDatabaseFactory> {
    public:
        DominanceDatabaseFactoryCategoryPlugin() : TypedCategoryPlugin("DominanceDatabaseFactory") {
            document_synopsis(
                    "This page describes the various data structures to store the set of explored states to check dominance against."
            );
        }
    } _category_plugin;

}