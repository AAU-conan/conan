#ifndef NFA_MERGE_NON_DIFFERENTIABLE_H
#define NFA_MERGE_NON_DIFFERENTIABLE_H

#include <mata/nfa/nfa.hh>
#include <vector>

namespace qdominance {
    class QualifiedLocalStateRelation;
    std::pair<mata::nfa::Nfa, std::vector<mata::nfa::State>> merge_non_differentiable_states(const mata::nfa::Nfa& nfa);

    [[nodiscard]] mata::nfa::Nfa minimize2(const mata::nfa::Nfa& nfa);

    [[nodiscard]] std::pair<mata::nfa::Nfa, std::vector<mata::nfa::State>> minimize3(const mata::nfa::Nfa& nfa);
}

#endif //NFA_MERGE_NON_DIFFERENTIABLE_H
