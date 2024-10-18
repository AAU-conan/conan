#ifndef GRAPHVIZ_H
#define GRAPHVIZ_H

#include <cassert>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <ranges>

namespace graphviz {
    class Graph {
    private:
        struct pair_hash {
            template <class T1, class T2>
            std::size_t operator () (const std::pair<T1, T2> &pair) const {
                return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
            }
        };

        class EdgeContainer {
        protected:
            static std::string format_attrs(const std::string& label, const std::string &attrs) {
                return "label=\"" + label + (attrs.empty()? "": ",") + attrs + "\"";
            }
        public:
            virtual ~EdgeContainer() = default;
            virtual void add_edge(size_t from, size_t to, const std::string &label, const std::string &attrs) = 0;
            [[nodiscard]] virtual std::vector<std::tuple<size_t, size_t, const std::string&>> get_edges() const = 0;
        };

        class SeparateEdgeContainer : public EdgeContainer {
            // There will be a separate edge for each added edge.
            std::vector<std::tuple<size_t, size_t, std::string>> edges;

            void add_edge(size_t from, size_t to, const std::string &label, const std::string &attrs) override {
                edges.emplace_back(from, to, format_attrs(label, attrs));
            }

            [[nodiscard]] std::vector<std::tuple<size_t, size_t, const std::string&>> get_edges() const override {
                std::vector<std::tuple<size_t, size_t, const std::string&>> result;
                result.reserve(edges.size());
                for (const auto &edge : edges) {
                    result.emplace_back(edge);
                }
                return result;
            }
        };

        class AggregatedEdgeContainer : public EdgeContainer {
            // There will be a single edge for each unique pair of from and to nodes.
            std::unordered_map<std::pair<size_t, size_t>, std::pair<std::string, std::string>, pair_hash> edges;

            void add_edge(size_t from, size_t to, const std::string &label, const std::string &attrs) override {
                auto p = std::make_pair(from, to);
                if (auto it = edges.find(p); it != edges.end()) {
                    if (attrs != it->second.second) {
                        std::cerr << "Warning: Different attributes for the same edge: " << from << " -> " << to << std::endl;
                    }
                    it->second = std::make_pair(it->second.first + ",\n" + label, attrs);
                } else {
                    edges[p] = std::make_pair(label, attrs);
                }
            }

            [[nodiscard]] std::vector<std::tuple<size_t, size_t, const std::string&>> get_edges() const override {
                return edges | std::views::transform([](const auto &pair) {
                    return std::make_tuple(pair.first.first, pair.first.second, format_attrs(pair.second.first, pair.second.second));
                }) | std::ranges::to<std::vector<std::tuple<size_t, size_t, const std::string&>>>();
            }
        };


        std::vector<std::string> node_attrs;
        std::unique_ptr<EdgeContainer> edges;

    public:
        explicit Graph(bool aggregate_labels = false) {
            if (aggregate_labels) {
                edges = std::make_unique<AggregatedEdgeContainer>();
            } else {
                edges = std::make_unique<SeparateEdgeContainer>();
            }
        }

        size_t add_node(const std::string &label, const std::string& attrs =  "");
        void add_edge(size_t from, size_t to, const std::string &label, const std::string &attrs = "");
        void output_graph(const std::string &filename) const;
    };
};



#endif //GRAPHVIZ_H
