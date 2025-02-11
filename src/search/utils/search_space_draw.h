#ifndef UTILS_SEARCH_SPACE_DRAW_H
#define UTILS_SEARCH_SPACE_DRAW_H

#include <unordered_map>

#include <boost/container_hash/hash.hpp>

class State;
class AbstractTask;
class OperatorProxy;

namespace fts {
    class FactNames;
}


namespace utils {
    template <typename Container>
    struct container_hash {
        std::size_t operator()(Container const& c) const {
            return boost::hash_range(c.begin(), c.end());
        }
    };

    class SearchSpaceDrawer {
        struct Edge {
            int from_id;
            int to_id;
            OperatorProxy op;
        };

        int state_lookup(const State& state);
        static std::string state_name(const std::vector<int>& state_vector, const fts::FactNames& names);
        const StateRegistry* state_registry = nullptr;
        std::vector<Edge> edges;
        std::unordered_map<std::vector<int>, int, container_hash<std::vector<int>>> state_to_id;
        std::unordered_map<int, int> heuristic_values;

        std::string output_path;

    public:
        explicit SearchSpaceDrawer(std::string output_path);

        void set_initial_state(const State &initial_state);

        void add_successor(const State &state, const OperatorProxy &op, const State &successor_state);

        void set_heuristic_value(const State &state, int value);

        void draw_search_space(const std::shared_ptr<AbstractTask>& task);
    };
}

#endif //SEARCH_SPACE_DRAW_H
