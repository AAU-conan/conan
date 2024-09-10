#include "draw_fts.h"
#include "../utils/graphviz.h"
#include "fts_task.h"

namespace fts {
    void draw_fts(const std::string &filename, const FTSTask &fts) {
        graphviz::Graph graph;
        for (const auto& [i, lts] : std::views::enumerate(fts.get_factors())) {
            std::unordered_map<AbstractStateRef, size_t> state_to_node;
            for (int j = 0; j < lts->size(); ++j) {
                state_to_node[j] = graph.add_node(fts.get_fact_name(FactPair(i, j)));
            }
            for (auto &t : lts->get_transitions())
            {
                graph.add_edge(state_to_node[t.src], state_to_node[t.target], std::to_string(t.label_group.group));
            }
        }
        graph.output_graph(filename);
    }
}