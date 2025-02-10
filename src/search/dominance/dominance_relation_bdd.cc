#include "dominance_relation_bdd.h"

#include "../factored_transition_system/symbolic_state_mapping.h"

#include <cassert>
using namespace std;

namespace dominance {
    DominanceRelationBDD::DominanceRelationBDD(const StateDominanceRelation &dominance_relation,
                                               const fts::FactoredSymbolicStateMapping &symbolic_mapping,
                                               bool dominated): dominated(dominated) {
        for (size_t i = 0; i < dominance_relation.size(); ++i) {
            if (dominated) {
                local_bdd_representation.push_back(
                    LocalStateRelationBDD::precompute_dominated_bdds(symbolic_mapping[i],
                                                                     *(dominance_relation.get_local_relations()[i])));
            } else {
                local_bdd_representation.push_back(
                    LocalStateRelationBDD::precompute_dominating_bdds(symbolic_mapping[i],
                                                                      *(dominance_relation.get_local_relations()[i])));
            }
        }
    }

    BDD DominanceRelationBDD::get_related_states(const ExplicitState &state) const {
        assert(!local_bdd_representation.empty());
        try {
            //TODO: This should be done in the variable order
            BDD res = local_bdd_representation[0]->get_dominance_bdd(state[0]);
            for (size_t i = 1; i < local_bdd_representation.size(); ++i) {
                res *= local_bdd_representation[i]->get_dominance_bdd(state[i]);
            }
            return res;
        } catch (symbolic::BDDError) {
            // This should never happen.
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }


    unique_ptr<LocalStateRelationBDD>
    LocalStateRelationBDD::precompute_dominating_bdds(const fts::SymbolicStateMapping &symbolic_mapping,
                                                      const dominance::FactorDominanceRelation &state_relation) {
        std::vector<BDD> dominance_bdds;
        const int num_states = state_relation.get_lts().size();
        dominance_bdds.reserve(num_states);
        for (int i = 0; i < num_states; ++i) {
            assert(state_relation.simulates(i, i));
            BDD dominance = symbolic_mapping.get_bdd(i);
            for (int j = 0;
                 j < num_states; ++j) {
                //TODO: Here we could iterate over the dominating states
                if (state_relation.simulates(j, i) && i != j) {
                    dominance += symbolic_mapping.get_bdd(j);
                }
            }
            dominance_bdds.push_back(dominance);
        }
        return std::make_unique<LocalStateRelationBDD>(std::move(dominance_bdds));
    }


    unique_ptr<LocalStateRelationBDD>
    LocalStateRelationBDD::precompute_dominated_bdds(const fts::SymbolicStateMapping &symbolic_mapping,
                                                     const dominance::FactorDominanceRelation &state_relation) {
        std::vector<BDD> dominance_bdds;
        const int num_states = state_relation.get_lts().size();
        dominance_bdds.reserve(num_states);
        for (int i = 0; i < num_states; ++i) {
            assert(state_relation.simulates(i, i));
            BDD dominance = symbolic_mapping.get_bdd(i);
            for (int j = 0; j < num_states; ++j) {
                if (state_relation.simulates(i, j) && i != j) {
                    dominance += symbolic_mapping.get_bdd(j);
                }
            }
            dominance_bdds.push_back(dominance);
        }
        assert (dominance_bdds.size() == static_cast<size_t>(num_states));
        return std::make_unique<LocalStateRelationBDD>(std::move(dominance_bdds));
    }
}
