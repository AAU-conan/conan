#include "dominance_analysis.h"

#include "../plugins/plugin.h"

namespace dominance {
    static class DominanceAnalysisCategoryPlugin : public plugins::TypedCategoryPlugin<DominanceAnalysis> {
    public:
        DominanceAnalysisCategoryPlugin() : TypedCategoryPlugin("DominanceAnalysis") {
            document_synopsis(
                    "This page describes the various methods to compute a dominance relation on a factored task."
            );
        }
    } _category_plugin;

}