#ifndef GRAPHVIZ_H
#define GRAPHVIZ_H

#include <cassert>
#include <format>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <generator>

#include "../lp/soplex_solver_interface.h"

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
                return std::format("label=\"{}\"{}{}", label, attrs.empty()? "": ",", attrs);
            }
        public:
            virtual ~EdgeContainer() = default;
            virtual void add_edge(size_t from, size_t to, const std::string &label, const std::string &attrs) = 0;
            [[nodiscard]] virtual std::generator<const std::tuple<size_t, size_t, const std::string&>> get_edges() const = 0;
        };

        class SeparateEdgeContainer : public EdgeContainer {
            // There will be a separate edge for each added edge.
            std::vector<std::tuple<size_t, size_t, std::string>> edges;

            void add_edge(size_t from, size_t to, const std::string &label, const std::string &attrs) override {
                edges.emplace_back(from, to, format_attrs(label, attrs));
            }

            [[nodiscard]] std::generator<const std::tuple<size_t, size_t, const std::string&>> get_edges() const override {
                for (const auto &edge : edges) {
                    co_yield edge;
                }
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
                    it->second = std::make_pair(std::format("{},\n{}", it->second.first, label), attrs);
                } else {
                    edges[p] = std::make_pair(label, attrs);
                }
            }

            [[nodiscard]] std::generator<const std::tuple<size_t, size_t, const std::string&>> get_edges() const override {
                for (const auto &[pair, label_attrs] : edges) {
                    co_yield std::make_tuple(pair.first, pair.second, format_attrs(label_attrs.first, label_attrs.second));
                }
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
