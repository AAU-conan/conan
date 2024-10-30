#ifndef DETERMINISTIC_TRANSITION_RESPONSE_H
#define DETERMINISTIC_TRANSITION_RESPONSE_H

#include <boost/bimap.hpp>
#include <map>
#include <set>

#include "../factored_transition_system/labelled_transition_system.h"


class TransitionResponse {
    typedef size_t Node;

    // The s state is always deterministic, the t is a set of t states
    struct CompoundState {
        int s_state;
        std::set<int> t_states;
    };

    struct Edge {
        Node source;
        Node target;
        int label;
    };

    typedef boost::bimap<CompoundState, Node> csn_bimap;
    csn_bimap compound_state_to_node;

    std::vector<Edge> edges;

    Node get_node(const CompoundState& cs) {
        if (compound_state_to_node.left.find(cs) != compound_state_to_node.left.end()) {
            return compound_state_to_node.left.at(cs);
        } else {
            Node new_node = compound_state_to_node.size();
            compound_state_to_node.insert({cs, new_node});
            return new_node;
        }
    }

public:
    TransitionResponse(const std::vector<std::vector<std::vector<std::vector<size_t>>>>& lts_transition_response, const fts::LabelledTransitionSystem &lts);
};

#endif //DETERMINISTIC_TRANSITION_RESPONSE_H
