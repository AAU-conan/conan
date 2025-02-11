#ifndef MERGE_AND_SHRINK_LABEL_RELATION_H
#define MERGE_AND_SHRINK_LABEL_RELATION_H

#include <memory>

namespace fts {
    class FTSTask;
    class LabelledTransitionSystem;
    class LabelMap;
}

namespace utils {
    class LogProxy;
}

namespace dominance {
    class FactorDominanceRelation;

    /*
     * Label relation represents the preorder relations on labels that
     * occur in a set of LTS
     */
    class LabelRelation {
    protected:
        int num_labels;
        // TODO: remove fts_task
        const fts::FTSTask& fts_task;

    public:
        explicit LabelRelation(const fts::FTSTask& fts_task);
        virtual ~LabelRelation() = default;

        [[nodiscard]] virtual bool label_dominates_label_in_all_other(int factor, int l1, int l2) const = 0;

        [[nodiscard]] virtual bool noop_simulates_label_in_all_other(int factor, int l) const = 0;

        virtual bool update_factor(int factor, const FactorDominanceRelation& sim) = 0;


        void dump(utils::LogProxy &log) const;
    };

    class LabelRelationFactory {
    public:
        virtual ~LabelRelationFactory() = default;
        virtual std::unique_ptr<LabelRelation> create(const fts::FTSTask& fts_task) = 0;
    };

    template<class LabelRelationType>
    class LabelRelationFactoryImpl final : public LabelRelationFactory {
        std::unique_ptr<LabelRelation> create(const fts::FTSTask& fts_task) override {
            return std::make_unique<LabelRelationType>(fts_task);
        }
    };
}
    

#endif
