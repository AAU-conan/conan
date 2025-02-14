#ifndef QUALIFIED_DOMINANCE_NFA_MERGE_NON_DIFFERENTIABLE_H
#define QUALIFIED_DOMINANCE_NFA_MERGE_NON_DIFFERENTIABLE_H

#include <mata/nfa/nfa.hh>
#include <vector>

namespace dominance {
    class SparseFactorRelation;
    std::pair<mata::nfa::Nfa, std::vector<mata::nfa::State>> merge_non_differentiable_states(const mata::nfa::Nfa& nfa, bool is_deterministic = false);

    [[nodiscard]] mata::nfa::Nfa minimize2(const mata::nfa::Nfa& nfa);
    [[nodiscard]] std::pair<mata::nfa::Nfa, std::vector<mata::nfa::State>> minimize_determinize_hopcroft(const mata::nfa::Nfa& nfa, bool is_deterministic = false);
    [[nodiscard]] mata::nfa::Nfa determinize(const mata::nfa::Nfa& aut);

    [[nodiscard]] std::pair<mata::nfa::Nfa, std::vector<mata::nfa::State>> minimize_hopcroft(const mata::nfa::Nfa& nfa);
}

#endif
