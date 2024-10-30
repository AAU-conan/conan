#include "transition_response.h"

TransitionResponse::TransitionResponse(int s_init, int t_init, const std::vector<std::vector<std::vector<std::vector<size_t>>>>& lts_transition_response, const fts::LabelledTransitionSystem& lts) {

    std::set<Node> waiting = {get_node(CompoundState{s_init, {t_init}})}; // Nodes that are waiting to be processed
    std::set<Node> seen = {get_node(CompoundState{s_init, {t_init}})}; // All nodes that have been seen

    while (!waiting.empty()) {
        auto n = *waiting.begin();
        waiting.erase(n);
        CompoundState cs = compound_state_to_node.right.at(n);

        const auto& s_transitions = lts.get_transitions(cs.s_state);
        for (auto s_tr_i = 0; s_tr_i < s_transitions.size(); ++s_tr_i) {
            CompoundState target_state;
            for (const auto& t : cs.t_states) {
                const auto& t_transitions = lts.get_transitions(t);
                for (auto t_tr_i : lts_transition_response[cs.s_state][t][s_tr_i]) {
                    if (t_tr_i == -1) {
                        target_state.t_states.insert(t);
                    } else {
                        target_state.t_states.insert(t_transitions[t_tr_i].target);
                    }
                }
            }
            edges.emplace_back(
                    get_node(CompoundState{s_init, {t_init}}),
                    get_node(target_state),
                    s_transitions[s_tr_i].label_group.group
            );
            if (!seen.contains(get_node(target_state))) {
                if (!target_state.t_states.empty()) {
                    waiting.insert(get_node(target_state));
                }
                seen.insert(get_node(target_state));
            }
        }
    }

    std::vector<std::vector<Node>> node_premap; // For each node, the nodes that has an edge to it
    node_premap.resize(compound_state_to_node.size());
    for (const auto& edge : edges) {
        node_premap[edge.target].push_back(edge.source);
    }


}
