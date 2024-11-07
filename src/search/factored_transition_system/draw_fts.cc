#include "draw_fts.h"
#include "../utils/graphviz.h"
#include "fts_task.h"

namespace fts {
    void draw_fts(const std::string &filename, const FTSTask &fts) {
        graphviz::Graph graph(true);
        for (const auto& [i, lts] : std::views::enumerate(fts.get_factors())) {
            std::unordered_map<AbstractStateRef, size_t> state_to_node;
            for (int j = 0; j < lts->size(); ++j) {
                state_to_node[j] = graph.add_node(fts.get_fact_name(FactPair(i, j)), lts->is_goal(j) ? "peripheries=2" : "");
            }
            for (auto &t : lts->get_transitions())
            {
                if (lts->is_relevant_label_group(t.label_group)) {
                    graph.add_edge(state_to_node[t.src], state_to_node[t.target], lts->label_group_name(t.label_group));
                }
            }
        }
        graph.output_graph(filename);
    }
}