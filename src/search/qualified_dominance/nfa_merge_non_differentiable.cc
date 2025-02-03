#include <set>
#include <print>
#include <boost/dynamic_bitset.hpp>

#include "nfa_merge_non_differentiable.h"

#include <boost/lexical_cast/detail/converter_numeric.hpp>

#include "sparse_local_state_relation.h"
#include "../algorithms/dynamic_bitset.h"
#include "../utils/logging.h"



using namespace mata::nfa;
using Symbol = mata::Symbol;

namespace dominance {
    [[nodiscard]] Nfa determinize(const Nfa& aut) {
        Nfa result{};
        result.alphabet = aut.alphabet;
        //assuming all sets targets are non-empty
        std::vector<std::pair<State, StateSet>> worklist{};
        std::unordered_map<StateSet, State> subset_map_local{};
        auto subset_map = &subset_map_local;

        for (mata::nfa::State s{0}; s < aut.num_of_states(); ++s) {
            const StateSet S{ s };
            const State Sid{ result.add_state() };

            if (aut.final.intersects_with(S)) {
                result.final.insert(Sid);
            }
            worklist.emplace_back(Sid, S);
            (*subset_map)[mata::utils::OrdVector<State>(S)] = Sid;
        }
        if (aut.delta.empty()) { return result; }

        using Iterator = mata::utils::OrdVector<SymbolPost>::const_iterator;
        SynchronizedExistentialSymbolPostIterator synchronized_iterator;

        while (!worklist.empty()) {
            const auto Spair = worklist.back();
            worklist.pop_back();
            const StateSet S = Spair.second;
            const State Sid = Spair.first;
            if (S.empty()) {
                // This should not happen assuming all sets targets are non-empty.
                break;
            }

            // add moves of S to the sync ex iterator
            synchronized_iterator.reset();
            for (State q: S) {
                mata::utils::push_back(synchronized_iterator, aut.delta[q]);
            }

            while (synchronized_iterator.advance()) {
                // extract post from the synchronized_iterator iterator
                const std::vector<Iterator>& symbol_posts = synchronized_iterator.get_current();
                mata::Symbol currentSymbol = (*symbol_posts.begin())->symbol;
                StateSet T = synchronized_iterator.unify_targets();

                const auto existingTitr = subset_map->find(T);
                State Tid;
                if (existingTitr != subset_map->end()) {
                    Tid = existingTitr->second;
                } else {
                    Tid = result.add_state();
                    (*subset_map)[mata::utils::OrdVector<State>(T)] = Tid;
                    if (aut.final.intersects_with(T)) {
                        result.final.insert(Tid);
                    }
                    worklist.emplace_back(Tid, T);
                }
                result.delta.mutable_state_post(Sid).insert(SymbolPost(currentSymbol, Tid));
            }
        }
        return result;
    }

    [[nodiscard]] std::pair<Nfa, std::vector<State>> merge_non_differentiable_states(const Nfa& nfa, bool is_deterministic) {
        assert(nfa.is_complete(nfa.alphabet)); // This check is not sufficient because it only checks reachable states
        Nfa detnfa(nfa);
        // DON'T USE nfa AFTERWARDS
        detnfa.make_complete();
        if (!is_deterministic) {
            detnfa = dominance::determinize(detnfa);
        }

        std::vector<std::vector<State>> partition = {{}, {}};
        std::vector<size_t> state_to_partition;
        for (State s = 0; s < detnfa.num_of_states(); ++s) {
            partition[detnfa.final.contains(s)? 1 : 0].push_back(s);
            state_to_partition.push_back(detnfa.final.contains(s)? 1 : 0);
        }
        std::set<size_t> waiting = {0, 1}; // Holds the indices of partition which is in waiting state

        // Maps each state s and label l, to the states that have an l-transition to s
        std::vector<std::vector<std::vector<State>>> state_label_premap = std::vector(detnfa.num_of_states(), std::vector(detnfa.alphabet->get_alphabet_symbols().size(), std::vector<State>(detnfa.num_of_states())));
        for (auto [from, label, to] : detnfa.delta.transitions()) {
            state_label_premap[to][label].push_back(from);
        }

        while (!waiting.empty()) {
            // Pop the first element from waiting
            auto a_index = *waiting.begin();
            waiting.erase(waiting.begin());

            for (mata::Symbol label : detnfa.alphabet->get_alphabet_symbols()) {
                std::set<State> states_leading_to_a;
                for (State s : partition.at(a_index)) {
                    for (State from : state_label_premap[s][label]) {
                        states_leading_to_a.insert(from);
                    }
                }

                for (size_t y_part_index = 0; y_part_index < partition.size(); ++y_part_index) {
                    std::vector<State> difference;
                    std::vector<State> intersection;
                    for (State s : partition[y_part_index]) {
                        if (states_leading_to_a.contains(s)) {
                            intersection.push_back(s);
                        } else {
                            difference.push_back(s);
                        }
                    }

                    if (!intersection.empty() && !difference.empty()) {
                        // y_part is split into intersection and difference
                        partition[y_part_index] = std::move(intersection); // This invalidates y_part
                        auto intersection_index = y_part_index;
                        partition.push_back(std::move(difference));
                        auto difference_index = partition.size() - 1;

                        if (waiting.contains(intersection_index)) {
                            // Fix the waiting list
                            // intersection will now have the y_part_index, and difference will be the last element in partition
                            waiting.insert(difference_index);
                        } else {
                            // Add the smaller of the two new partitions to the waiting list
                            if (intersection.size() < difference.size()) {
                                waiting.insert(intersection_index);
                            } else {
                                waiting.insert(difference_index);
                            }
                        }
                    }
                }
            }
        }


        // Create the new NFA
        Nfa new_nfa = Nfa({}, {}, {}, detnfa.alphabet);

        // Create new states and make old to new state mapping
        std::vector<State> old_to_new(detnfa.num_of_states(), -1);
        for (const auto& [new_index, new_part] : std::views::enumerate(partition)) {
            new_nfa.add_state();

            for (State old : new_part) {
                old_to_new[old] = new_index;
            }
        }

        // Add transitions
        for (const auto& [from, label, to] : detnfa.delta.transitions()) {
            new_nfa.delta.add(old_to_new[from], label, old_to_new[to]);
        }

        // Add initial states
        for (State initial : detnfa.initial) {
            new_nfa.initial.insert(old_to_new[initial]);
        }
        assert(new_nfa.initial.size() <= 1);

        // Add final states
        for (State final : detnfa.final) {
            new_nfa.final.insert(old_to_new[final]);
        }

        return {new_nfa, old_to_new};
    }

    // Anonymous namespace for the Hopcroft minimization algorithm.
    // This is stolen from the Verifit Mata library.
    namespace {
    template <typename T>
    /**
     * A class for partitionable sets of elements. All elements are stored in a contiguous vector.
     * Elements from the same set are contiguous in the vector. Auxiliary data (indices, positions, etc.) are
     * stored in separate vectors.
     */
    class RefinablePartition {
    public:
        static_assert(std::is_unsigned<T>::value, "T must be an unsigned type.");
        static const T NO_MORE_ELEMENTS = std::numeric_limits<T>::max();
        static const size_t NO_SPLIT = std::numeric_limits<size_t>::max();
        size_t num_of_sets;            ///< The number of sets in the partition.
        std::vector<size_t> set_idx;   ///< For each element, tells the index of the set it belongs to.

    private:
        std::vector<T> elems;       ///< Contains all elements in an order such that the elements of the same set are contiguous.
        std::vector<T> location;    ///< Contains the location of each element in the elements vector.
        std::vector<size_t> first;  ///< Contains the index of the first element of each set in the elements vector.
        std::vector<size_t> end;    ///< Contains the index after the last element of each set in the elements vector.
        std::vector<size_t> mid;    ///< Contains the index after the last element of the first half of the set in the elements vector.

    public:
        /**
         * @brief Construct a new Refinable Partition for equivalence classes of states.
         *
         * @param n The number of states.
         */
        RefinablePartition(const size_t num_of_states)
            : num_of_sets(1), set_idx(num_of_states), elems(num_of_states), location(num_of_states),
              first(num_of_states), end(num_of_states), mid(num_of_states) {
            // Initially, all states are in the same equivalence class.
            first[0] = mid[0] = 0;
            end[0] = num_of_states;
            for (State e = 0; e < num_of_states; ++e) {
                elems[e] = location[e] = e;
                set_idx[e] = 0;
            }
        }

        /**
         * @brief Construct a new Refinable Partition for sets of splitters (transitions incoming to
         *  equivalence classes under a common symbol). Initially, the partition has n sets, where n is the alphabet size.
         *
         * @param delta The transition function.
         */
        RefinablePartition(const Delta &delta)
            : num_of_sets(0), set_idx(), elems(), location(), first(), end(), mid() {
            size_t num_of_transitions = 0;
            std::vector<size_t> counts;
            std::unordered_map<Symbol, size_t> symbol_map;

            // Transitions are grouped by symbols using counting sort in time O(m).
            // Count the number of elements and the number of sets.
            for (const auto &trans : delta.transitions()) {
                const Symbol a = trans.symbol;
                if (symbol_map.find(a) == symbol_map.end()) {
                    symbol_map[a] = num_of_sets++;
                    counts.push_back(1);
                } else {
                    ++counts[symbol_map[a]];
                }
                ++num_of_transitions;
            }

            // Initialize data structures.
            elems.resize(num_of_transitions);
            location.resize(num_of_transitions);
            set_idx.resize(num_of_transitions);
            first.resize(num_of_transitions);
            end.resize(num_of_transitions);
            mid.resize(num_of_transitions);

            // Compute set indices.
            // Use (mid - 1) as an index for the insertion.
            first[0] = 0;
            end[0] = counts[0];
            mid[0] = end[0];
            for (size_t i = 1; i < num_of_sets; ++i) {
                first[i] = end[i - 1];
                end[i] = first[i] + counts[i];
                mid[i] = end[i];
            }

            // Fill the sets from the back.
            // Mid, decremented before use, is used as an index for the next element.
            size_t trans_idx = 0;   // Index of the transition in the (flattened) delta.
            for (const auto &trans : delta.transitions()) {
                const Symbol a = trans.symbol;
                const size_t a_idx = symbol_map[a];
                const size_t trans_loc = mid[a_idx] - 1;
                mid[a_idx] = trans_loc;
                elems[trans_loc] = trans_idx;
                location[trans_idx] = trans_loc;
                set_idx[trans_idx] = a_idx;
                ++trans_idx;
            }
        }

        RefinablePartition(const RefinablePartition &other)
            : elems(other.elems), set_idx(other.set_idx), location(other.location),
              first(other.first), end(other.end), mid(other.mid), num_of_sets(other.num_of_sets) {}

        RefinablePartition(RefinablePartition &&other) noexcept
            : elems(std::move(other.elems)), set_idx(std::move(other.set_idx)), location(std::move(other.location)),
              first(std::move(other.first)), end(std::move(other.end)), mid(std::move(other.mid)), num_of_sets(other.num_of_sets) {}

        RefinablePartition& operator=(const RefinablePartition &other) {
            if (this != &other) {
                elems = other.elems;
                set_idx = other.set_idx;
                location = other.location;
                first = other.first;
                end = other.end;
                mid = other.mid;
                num_of_sets = other.num_of_sets;
            }
            return *this;
        }

        RefinablePartition& operator=(RefinablePartition &&other) noexcept {
            if (this != &other) {
                elems = std::move(other.elems);
                set_idx = std::move(other.set_idx);
                location = std::move(other.location);
                first = std::move(other.first);
                end = std::move(other.end);
                mid = std::move(other.mid);
                num_of_sets = other.num_of_sets;
            }
            return *this;
        }

        /**
         * @brief Get the number of elements across all sets.
         *
         * @return The number of elements.
         */
        inline size_t size() const { return elems.size(); }

        /**
         * @brief Get the size of the set.
         *
         * @param s The set index.
         * @return The size of the set.
         */
        inline size_t size_of_set(size_t s) const { return end[s] - first[s]; }

        /**
         * @brief Get the first element of the set.
         *
         * @param s The set index.
         * @return The first element of the set.
         */
        inline T get_first(const size_t s) const { return elems[first[s]]; }

        /**
         * @brief Get the next element of the set.
         *
         * @param e The element.
         * @return The next element of the set or NO_MORE_ELEMENTS if there is no next element.
         */
        inline T get_next(const T e) const {
            if (location[e] + 1 >= end[set_idx[e]]) { return NO_MORE_ELEMENTS; }
            return elems[location[e] + 1];
        }

        /**
         * @brief Mark the element and move it to the first half of the set.
         *
         * @param e The element.
         */
        void mark(const T e) {
            const size_t e_set = set_idx[e];
            const size_t e_loc = location[e];
            const size_t e_set_mid = mid[e_set];
            if (e_loc >= e_set_mid) {
                elems[e_loc] = elems[e_set_mid];
                location[elems[e_loc]] = e_loc;
                elems[e_set_mid] = e;
                location[e] = e_set_mid;
                mid[e_set] = e_set_mid + 1;
            }
        }

        /**
         * @brief Test if the set has no marked elements.
         *
         * @param s The set index.
         * @return True if the set has no marked elements, false otherwise.
         */
        inline bool has_no_marks(const size_t s) const { return mid[s] == first[s]; }

        /**
         * @brief Split the set into two sets according to the marked elements (the mid).
         * The first set name remains the same.
         *
         * @param s The set index.
         * @return The new set index.
         */
        size_t split(const size_t s) {
            if (mid[s] == end[s]) {
                // If no elements were marked, move the mid to the end (no split needed).
                mid[s] = first[s];
            }
            if (mid[s] == first[s]) {
                // If all or no elements were marked, there is no need to split the set.
                return NO_SPLIT;
            } else {
                // Split the set according to the mid.
                first[num_of_sets] = first[s];
                mid[num_of_sets] = first[s];
                end[num_of_sets] = mid[s];
                first[s] = mid[s];
                for (size_t l = first[num_of_sets]; l < end[num_of_sets]; ++l) {
                    set_idx[elems[l]] = num_of_sets;
                }
                return num_of_sets++;
            }
        }
    };
    } // namespace


    std::pair<Nfa, std::vector<State>> minimize_hopcroft(const Nfa& dfa_trimmed) {
        if (dfa_trimmed.delta.num_of_transitions() == 0) {
            // The automaton is trivially minimal.
            return {Nfa{ dfa_trimmed }, std::views::iota(dfa_trimmed.num_of_states()) | std::ranges::to<std::vector<State>>()};
        }
        // assert(dfa_trimmed.is_deterministic());
        // assert(dfa_trimmed.get_useful_states().size() == dfa_trimmed.num_of_states());

        // Initialize equivalence classes. The initial partition is {Q}.
        RefinablePartition<State> brp(dfa_trimmed.num_of_states());
        // Initialize splitters. A splitter is a set of transitions
        // over a common symbol incoming to an equivalence class. Initially,
        // the partition has m splitters, where m is the alphabet size.
        RefinablePartition<size_t> trp(dfa_trimmed.delta);

        // Initialize vector of incoming transitions for each state. Transitions
        // are represented only by their indices in the (flattened) delta.
        // Initialize mapping from transition index to its source state.
        std::vector<State> trans_source_map(trp.size());
        std::vector<std::vector<size_t>> incomming_trans_idxs(brp.size(), std::vector<size_t>());
        size_t trans_idx = 0;
        for (const auto &trans : dfa_trimmed.delta.transitions()) {
            trans_source_map[trans_idx] = trans.source;
            incomming_trans_idxs[trans.target].push_back(trans_idx);
            ++trans_idx;
        }

        // Worklists for the Hopcroft algorithm.
        std::stack<size_t> unready_spls;    // Splitters that will be used in the backpropagation.
        std::stack<size_t> touched_blocks;  // Blocks (equivalence classes) touched during backpropagation.
        std::stack<size_t> touched_spls;    // Splitters touched (in the split_block function) as a result of backpropagation.

        /**
         * @brief Split the block (equivalence class) according to the marked states.
         *
         * @param b The block index.
         */
        auto split_block = [&](size_t b) {
            // touched_spls has been moved to a higher scope to avoid multiple constructions/destructions.
            assert(touched_spls.empty());
            size_t b_prime = brp.split(b);  // One block will keep the old name 'b'.
            if (b_prime == RefinablePartition<size_t>::NO_SPLIT) {
                // All or no states in the block were marked (touched) during the backpropagation.
                return;
            }
            // According to the Hopcroft's algorithm, it is sufficient to work only with the smaller block.
            if (brp.size_of_set(b) < brp.size_of_set(b_prime)) {
                b_prime = b;
            }
            // Split the transitions of the splitters according to the new partitioning.
            // Transitions in one splitter must have the same symbol and go to the same block.
            for (State q = brp.get_first(b_prime); q != RefinablePartition<State>::NO_MORE_ELEMENTS; q = brp.get_next(q)) {
                for (const size_t trans_idx : incomming_trans_idxs[q]) {
                    const size_t splitter_idx = trp.set_idx[trans_idx];
                    if (trp.has_no_marks(splitter_idx)) {
                        touched_spls.push(splitter_idx);
                    }
                    // Mark the transition in the splitter and move it to the first half of the set.
                    trp.mark(trans_idx);
                }
            }
            // Refine all splitters where some transitions were marked.
            while (!touched_spls.empty()) {
                const size_t splitter_idx = touched_spls.top();
                touched_spls.pop();
                const size_t splitter_pime = trp.split(splitter_idx);
                if (splitter_pime != RefinablePartition<size_t>::NO_SPLIT) {
                    // Use the new splitter for further refinement of the equivalence classes.
                    unready_spls.push(splitter_pime);
                }
            }
        };

        // Use all splitters for the initial refinement.
        for (size_t splitter_idx = 0; splitter_idx < trp.num_of_sets; ++splitter_idx) {
            unready_spls.push(splitter_idx);
        }

        // In the first refinement, we split the equivalence classes according to the final states.
        for (const State q : dfa_trimmed.final) {
            brp.mark(q);
        }
        split_block(0);

        // Main loop of the Hopcroft's algorithm.
        while (!unready_spls.empty()) {
            const size_t splitter_idx = unready_spls.top();
            unready_spls.pop();
            // Backpropagation.
            // Fire back all transitions of the splitter. (Transitions over the same
            // symbol that go to the same block.) Mark the source states of these transitions.
            for (size_t trans_idx = trp.get_first(splitter_idx); trans_idx != RefinablePartition<size_t>::NO_MORE_ELEMENTS; trans_idx = trp.get_next(trans_idx)) {
                const State q = trans_source_map[trans_idx];
                const size_t b_prime = brp.set_idx[q];
                if (brp.has_no_marks(b_prime)) {
                    touched_blocks.push(b_prime);
                }
                brp.mark(q);
            }
            // Try to split the blocks touched during the backpropagation.
            // The block will be split only if some states (not all) were touched (marked).
            while (!touched_blocks.empty()) {
                const size_t b = touched_blocks.top();
                touched_blocks.pop();
                split_block(b);
            }
        }

        // Construct the minimized automaton using equivalence classes (BRP).
        // assert(dfa_trimmed.initial.size() == 1);
        Nfa result(brp.num_of_sets, { }, {});
        for (size_t block_idx = 0; block_idx < brp.num_of_sets; ++block_idx) {
            const State q = brp.get_first(block_idx);
            if (dfa_trimmed.final.contains(q)) {
                result.final.insert(block_idx);
            }
            StatePost &mut_state_post = result.delta.mutable_state_post(block_idx);
            for (const SymbolPost &symbol_post : dfa_trimmed.delta[q]) {
                assert(symbol_post.targets.size() == 1);
                const State target = brp.set_idx[*symbol_post.targets.begin()];
                mut_state_post.push_back(SymbolPost{ symbol_post.symbol, StateSet{ target } });
            }
        }

        std::vector<State> old_to_new(dfa_trimmed.num_of_states());
        for (size_t block_idx = 0; block_idx < brp.num_of_sets; ++block_idx) {
            State q = brp.get_first(block_idx);
            while (q != RefinablePartition<State>::NO_MORE_ELEMENTS) {
                old_to_new[q] = block_idx;
                q = brp.get_next(q);
            }
        }

        return {result, old_to_new};
    }

    std::pair<Nfa, std::vector<State>> minimize_determinize_hopcroft(const Nfa& nfa, bool is_deterministic) {
        auto det_nfa = !is_deterministic? dominance::determinize(nfa): nfa;
        auto [min_nfa, old_to_new_map] = minimize_hopcroft(det_nfa);
        min_nfa.alphabet = nfa.alphabet;
        return {min_nfa, old_to_new_map};
    }
}