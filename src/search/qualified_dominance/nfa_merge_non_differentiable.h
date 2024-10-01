#ifndef NFA_MERGE_NON_DIFFERENTIABLE_H
#define NFA_MERGE_NON_DIFFERENTIABLE_H

#include <mata/nfa/nfa.hh>
#include <vector>

namespace qdominance {
    class QualifiedLocalStateRelation;
    std::pair<mata::nfa::Nfa, std::vector<mata::nfa::State>> merge_non_differentiable_states(const mata::nfa::Nfa& detnfa, const QualifiedLocalStateRelation& relation);
}

#endif //NFA_MERGE_NON_DIFFERENTIABLE_H
