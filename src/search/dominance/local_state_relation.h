#ifndef DOMINANCE_LOCAL_STATE_RELATION_H
#define DOMINANCE_LOCAL_STATE_RELATION_H

#include <vector>
#include <string>
#include <iostream>
#include <memory>

#include "../factored_transition_system/labelled_transition_system.h"
#include "../utils/logging.h"

namespace merge_and_shrink{
    class TransitionSystem;
}



namespace dominance {

    class FactorDominanceRelation {
    protected:
        fts::LabelledTransitionSystem lts;

    public:
        explicit FactorDominanceRelation(const fts::LabelledTransitionSystem& lts)
            : lts(lts) {
        }

        virtual ~FactorDominanceRelation() = default;

        [[nodiscard]] virtual inline bool simulates(int s, int t) const = 0;
        [[nodiscard]] virtual inline bool similar(int s, int t) const = 0;

        [[nodiscard]] const fts::LabelledTransitionSystem &get_lts() const {
            return lts;
        }

        [[nodiscard]] virtual bool is_identity() const;

        virtual void dump(utils::LogProxy &log) const;

        [[nodiscard]] virtual int num_equivalences() const;

        [[nodiscard]] virtual int num_simulations() const;

        [[nodiscard]] virtual int num_different_states() const;

        virtual void cancel_simulation_computation() = 0;

        virtual bool applySimulations(std::function<bool(int s, int t)> &&f) const = 0;
        virtual bool removeSimulations(std::function<bool(int s, int t)> &&f) = 0;
    };

    class FactorDominanceRelationFactory {
    public:
        virtual ~FactorDominanceRelationFactory() = default;
        virtual std::unique_ptr<FactorDominanceRelation> create(const fts::LabelledTransitionSystem &lts) = 0;
    };

    template<class DominanceRelation>
    class FactorDominanceRelationFactoryImpl : public FactorDominanceRelationFactory {
        std::unique_ptr<FactorDominanceRelation> create(const fts::LabelledTransitionSystem &lts) override {
            return std::make_unique<DominanceRelation>(lts);
        }
    };


    class DenseLocalStateRelation final : public FactorDominanceRelation {
    protected:
        std::vector<std::vector<bool> > relation;

        // Vectors of states dominated/dominating by each state. Lazily computed when needed.
        std::vector<std::vector<int>> dominated_states, dominating_states;
        void compute_list_dominated_states();
    public:
        explicit DenseLocalStateRelation(const fts::LabelledTransitionSystem& lts);

        void cancel_simulation_computation() override;

        void remove(int s, int t) {
            relation[s][t] = false;
        }

        [[nodiscard]] bool simulates(int s, int t) const override {
            return !relation.empty() ? relation[s][t] : s == t;
        }

        [[nodiscard]] bool similar(int s, int t) const override {
            return !relation.empty() ?
                   relation[s][t] && relation[t][s] :
                   s == t;
        }

        inline const std::vector<std::vector<bool> > &get_relation() {
            return relation;
        }

        bool applySimulations(std::function<bool(int s, int t)> &&f) const override;
        bool removeSimulations(std::function<bool(int s, int t)> &&f) override;
    };
}
#endif
