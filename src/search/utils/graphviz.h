#ifndef GRAPHVIZ_H
#define GRAPHVIZ_H

#include <string>
#include <unordered_map>
#include <vector>

namespace graphviz {
    class Graph {
    private:
        std::vector<std::string> node_labels;

        struct pair_hash {
            template <class T1, class T2>
            std::size_t operator () (const std::pair<T1, T2> &pair) const {
                return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
            }
        };
        std::vector<std::tuple<size_t, size_t, std::string>> edges;

        bool aggregate_labels;

    public:
        explicit Graph(bool aggregate_labels = false) : aggregate_labels(aggregate_labels) {
        }

        size_t add_node(const std::string &label);
        void add_edge(size_t from, size_t to, const std::string &label);
        void output_graph(const std::string &filename) const;
    };
};



#endif //GRAPHVIZ_H
