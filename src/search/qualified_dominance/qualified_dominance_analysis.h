#ifndef QUALIFIED_DOMINANCE_DOMINANCE_ANALYSIS_H
#define QUALIFIED_DOMINANCE_DOMINANCE_ANALYSIS_H

#include <memory>

namespace fts {
    class FTSTask;
}

namespace qdominance {
    class QualifiedFactoredDominanceRelation;

    class QualifiedDominanceAnalysis {
    public:
        virtual ~QualifiedDominanceAnalysis() = default;

        virtual std::unique_ptr<QualifiedFactoredDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) = 0;
    };

}

#endif

