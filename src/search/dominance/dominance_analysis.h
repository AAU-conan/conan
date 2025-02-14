#ifndef DOMINANCE_DOMINANCE_ANALYSIS_H
#define DOMINANCE_DOMINANCE_ANALYSIS_H

#include <memory>

namespace fts {
    class FTSTask;
}

namespace dominance {
    class StateDominanceRelation;

    class DominanceAnalysis {
    public:
        virtual ~DominanceAnalysis() = default;

        virtual std::unique_ptr<StateDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) = 0;
    };

}

#endif

