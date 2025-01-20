
#include "../state_registry.h"
#include "search_space_draw.h"
#include "graphviz.h"
#include "../factored_transition_system/fact_names.h"
#include "../abstract_task.h"
#include "../state_id.h"

#include <boost/container_hash/hash.hpp>

struct Edge {
    StateID from_id;
    StateID to_id;
    OperatorProxy op;
};

static const StateRegistry* state_registry;

static std::vector<Edge> edges;

template <typename Container> // we can make this generic for any container [1]
struct container_hash {
    std::size_t operator()(Container const& c) const {
        return boost::hash_range(c.begin(), c.end());
    }
};

static std::unordered_map<std::vector<int>, int, container_hash<std::vector<int>>> state_to_id;

static std::unordered_map<int, int> heuristic_values;


int state_lookup(const State& state) {
    state.unpack();
    auto state_vec = state.get_unpacked_values();
    if (!state_to_id.contains(state_vec)) {
        state_to_id[state_vec] = state_to_id.size();
    }
    return state_to_id[state_vec];
}

void set_initial_state(const State& initial_state) {
    state_registry = initial_state.get_registry();
}

void add_successor(const State& state, const OperatorProxy& op, const State& successor_state) {
    edges.push_back({state.get_id(), successor_state.get_id(), op});
}

void set_heuristic_value(const State& state, int value) {
    int id = state_lookup(state);
    if (!heuristic_values.contains(id))
        heuristic_values[id] = value;
    else
        heuristic_values[id] = std::max(heuristic_values[id], value);
}

std::string state_name(const std::vector<int>& state_vector, const fts::FactNames& names) {
    std::string result;
    for (size_t i = 0; i < state_vector.size(); ++i) {
        if (i != 0) {
            result += ", ";
        }
        result += names.get_fact_name(FactPair(i, state_vector[i]));
    }
    return result;
}

void draw_search_space(const std::shared_ptr<AbstractTask>& task, const std::string& output_path) {
    graphviz::Graph graph;

    fts::AbstractTaskFactNames names(task);

    std::unordered_map<int, size_t> state_id_to_node;

    for (const auto& [state, id] : state_to_id) {
        std::string label = state_name(state, names);
        state_id_to_node[id] = graph.add_node(label, heuristic_values.contains(id)? std::format("xlabel=\"h={}\"", heuristic_values[id]): "");
    }

    for (const auto& [from_id, to_id, op] : edges) {
        const State& from_state = state_registry->lookup_state(from_id);
        const State& to_state = state_registry->lookup_state(to_id);

        graph.add_edge(state_id_to_node[state_lookup(from_state)], state_id_to_node[state_lookup(to_state)], names.get_operator_name(op.get_id(), op.is_axiom()));
    }

    graph.output_graph(output_path);
}
