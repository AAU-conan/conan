#ifndef DOMINANCE_DOMINANCE_ANALYSIS_H
#define DOMINANCE_DOMINANCE_ANALYSIS_H

#include <memory>

namespace fts {
    class FTSTask;
}

namespace dominance {
    class FactoredDominanceRelation;

    class DominanceAnalysis {
    public:
        virtual ~DominanceAnalysis() = default;

        virtual std::unique_ptr<FactoredDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) = 0;
    };

}

#endif

