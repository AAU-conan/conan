#ifndef DOMINANCE_DATABASE_H
#define DOMINANCE_DATABASE_H

#include <vector>
#include <memory>

class AbstractTask;

namespace fts {
    class FactoredStateMapping;
}

namespace dominance {
    class StateDominanceRelation;
    typedef std::vector<int> ExplicitState;

    class DominanceDatabase {
    public:
        virtual ~DominanceDatabase() = default;
        virtual bool check(const ExplicitState &state, int g) const = 0;
        virtual void insert(const ExplicitState &transformed_state, int g) = 0;
    };

    class DominanceDatabaseFactory {
    public:
        virtual ~DominanceDatabaseFactory() = default;
        virtual std::unique_ptr<DominanceDatabase> create(const std::shared_ptr<AbstractTask> &task,
                                                          std::shared_ptr<StateDominanceRelation> dominance_relation,
                                                          std::shared_ptr<fts::FactoredStateMapping> state_mapping) = 0;
    };
}
#endif
