#include "graphviz.h"
#include <cassert>
#include <iostream>
#include <fstream>

namespace graphviz {
    size_t Graph::add_node(const std::string &label) {
        node_labels.push_back(label);
        return node_labels.size() - 1;
    }

    void Graph::add_edge(size_t from, size_t to, const std::string &label) {
        assert(from < node_labels.size());
        assert(to < node_labels.size());
        edges.emplace_back(from, to, label);
    }

    void Graph::output_graph(const std::string &filename) const {
        std::ofstream file;
        file.open(filename);
        file << "digraph G {\n";
        for (size_t i = 0; i < node_labels.size(); ++i) {
            file << "  " << i << " [label=\"" << node_labels[i] << "\"];\n";
        }
        for (auto &[from, to, label] : edges) {
            file << "  " << from << " -> " << to << " [label=\"" << label << "\"];\n";
        }
        file << "}\n";
        file.close();
    }
}