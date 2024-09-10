#include "qualified_dominance_analysis.h"

#include "../plugins/plugin.h"

namespace qdominance {
    static class DominanceAnalysisCategoryPlugin : public plugins::TypedCategoryPlugin<QualifiedDominanceAnalysis> {
    public:
        DominanceAnalysisCategoryPlugin() : TypedCategoryPlugin("QualifiedDominanceAnalysis") {
            document_synopsis(
                    "This page describes the various methods to compute a qualified dominance relation on a factored task."
            );
        }
    } _category_plugin;

}