#include "draw_fts.h"

#include "fact_names.h"
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

            for (int s = 0; s < lts->size(); ++s) {
                std::unordered_map<int, std::vector<int>> state_labels;
                for (auto &t : lts->get_transitions(s)) {
                    if (lts->is_relevant_label_group(t.label_group)) {
                        for (int l : lts->get_labels(t.label_group)) {
                            state_labels[t.target].push_back(l);
                        }
                    }
                }
                for (const auto& [t, ls] : state_labels) {
                    graph.add_edge(state_to_node[s], state_to_node[t], lts->fact_value_names->get_common_operators_name(ls));
                }
            }
        }
        graph.output_graph(filename);
    }
}