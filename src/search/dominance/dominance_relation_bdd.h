#ifndef DOMINANCE_DOMINANCE_RELATION_BDD_H
#define DOMINANCE_DOMINANCE_RELATION_BDD_H

#include "factored_dominance_relation.h"
#include "../symbolic/bdd_manager.h"

namespace fts {
    class FactoredSymbolicStateMapping;

    class SymbolicStateMapping;
}

namespace dominance {
    class DenseLocalStateRelation;
    class FactoredDominanceRelation;

    typedef std::vector<int> ExplicitState;

    class LocalStateRelationBDD {
        std::vector<BDD> dominance_bdds;  //For each abstract state, we create a BDD that represents all the abstract states dominated by it or dominating it
    public:
        LocalStateRelationBDD(std::vector<BDD> &&dominance_bdds) :
                dominance_bdds(std::move(dominance_bdds)) {}

        BDD get_dominance_bdd(int value) const {
            return dominance_bdds[value];
        }

        static std::unique_ptr<LocalStateRelationBDD>
        precompute_dominating_bdds(const fts::SymbolicStateMapping &symbolic_mapping,
                                   const FactorDominanceRelation &state_relation);

        static std::unique_ptr<LocalStateRelationBDD>
        precompute_dominated_bdds(const fts::SymbolicStateMapping &symbolic_mapping,
                                  const FactorDominanceRelation &state_relation);
    };

    class DominanceRelationBDD {
        const bool dominated;
        std::vector<std::unique_ptr<LocalStateRelationBDD>> local_bdd_representation;

    public:
        DominanceRelationBDD (const FactoredDominanceRelation & dominance_relation,
                              const fts::FactoredSymbolicStateMapping &symbolic_mapping, bool dominated);

        BDD get_related_states(const ExplicitState &state) const;
    };
}

#endif
