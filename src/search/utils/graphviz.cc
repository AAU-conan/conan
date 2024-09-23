#include "graphviz.h"
#include <cassert>
#include <iostream>
#include <fstream>

namespace graphviz {
    size_t Graph::add_node(const std::string &label, const std::string &attrs) {
        node_attrs.push_back(std::format("label=\"{}\"{}{}", label, attrs.empty()? "": ",", attrs));
        return node_attrs.size() - 1;
    }

    void Graph::add_edge(size_t from, size_t to, const std::string &label, const std::string &attrs) {
        assert(from < node_attrs.size());
        assert(to < node_attrs.size());
        edges->add_edge(from, to, label, attrs);
    }

    void Graph::output_graph(const std::string &filename) const {
        std::ofstream file;
        file.open(filename);
        file << "digraph G {\n";
        for (size_t i = 0; i < node_attrs.size(); ++i) {
            file << "  " << i << " [" << node_attrs[i] << "];\n";
        }
        for (auto &[from, to, attrs] : edges->get_edges()) {
            file << "  " << from << " -> " << to << " [" << attrs << "];\n";
        }
        file << "}\n";
        file.close();
    }
}