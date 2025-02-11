
#include "../state_registry.h"
#include "search_space_draw.h"
#include "graphviz.h"
#include "../factored_transition_system/fact_names.h"
#include "../abstract_task.h"
#include "../state_id.h"



namespace utils {
    int SearchSpaceDrawer::state_lookup(const State& state) {
        state.unpack();
        const auto& state_vec = state.get_unpacked_values();
        if (!state_to_id.contains(state_vec)) {
            state_to_id[state_vec] = static_cast<int>(state_to_id.size());
        }
        return state_to_id[state_vec];
    }

    SearchSpaceDrawer::SearchSpaceDrawer(std::string output_path): output_path(std::move(output_path)) {}

    void SearchSpaceDrawer::set_initial_state(const State& initial_state) {
        state_registry = initial_state.get_registry();
    }

    void SearchSpaceDrawer::add_successor(const State& state, const OperatorProxy& op, const State& successor_state) {
        edges.push_back({state_lookup(state), state_lookup(successor_state), op});
    }

    void SearchSpaceDrawer::set_heuristic_value(const State& state, int value) {
        int id = state_lookup(state);
        if (!heuristic_values.contains(id))
            heuristic_values[id] = value;
        else
            heuristic_values[id] = std::max(heuristic_values[id], value);
    }

    std::string SearchSpaceDrawer::state_name(const std::vector<int>& state_vector, const fts::FactNames& names) {
        std::string result;
        for (int i = 0; i < static_cast<int>(state_vector.size()); ++i) {
            if (i != 0) {
                result += ", ";
            }
            result += names.get_fact_name(FactPair(i, state_vector[i]));
        }
        return result;
    }

    void SearchSpaceDrawer::draw_search_space(const std::shared_ptr<AbstractTask>& task) {
        graphviz::Graph graph;

        fts::AbstractTaskFactNames names(task);

        std::unordered_map<int, size_t> state_id_to_node;

        for (const auto& [state, id] : state_to_id) {
            std::string label = state_name(state, names);
            state_id_to_node[id] = graph.add_node(label, heuristic_values.contains(id)? std::format("xlabel=\"h={}\"", heuristic_values[id] == -1 ? "∞": std::format("{}", heuristic_values[id])): "");
        }

        for (const auto& [from_id, to_id, op] : edges) {
            graph.add_edge(state_id_to_node[from_id], state_id_to_node[to_id], names.get_operator_name(op.get_id(), op.is_axiom()));
        }

        graph.output_graph(output_path);
    }
}
