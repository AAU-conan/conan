#ifndef QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H
#define QUALIFIED_DOMINANCE_LOCAL_STATE_RELATION_H

#include <vector>
#include <string>

#include <rust/cxx.h>
#include <mata/nfa/nfa.hh>
#include <mata/nfa/types.hh>
#include "qualified_label_relation.h"
#include "../factored_transition_system/fact_names.h"
#include "../factored_transition_system/labelled_transition_system.h"

namespace merge_and_shrink{
    class TransitionSystem;
}

namespace fts {
    class FTSTask;
    class LabelledTransitionSystem;
}

namespace utils {
    class LogProxy;
}

namespace qdominance {
    using TransitionIndex = size_t;
    TransitionIndex constexpr NOOP_TRANSITION = -1;

    // First implementation of a simulation relation.
    class QualifiedLocalStateRelation2 {
    protected:

        fts::LabelledTransitionSystem lts;
        int factor;
        const fts::FTSTask& fts_task;
    public:

        // For each pair of states s_i, s_j and an s_i transition, if s_i is relevant the s_j transitions which simulate it otherwise none
        std::vector<std::vector<std::vector<std::vector<TransitionIndex>>>> transition_responses;

        // For each state, the set of states that it simulates
        std::vector<std::set<int>> simulations;

        // Vectors of states dominated/dominating by each state. Lazily computed when needed.
        std::vector<std::vector<int>> dominated_states, dominating_states;
        void compute_list_dominated_states();

        void cancel_simulation_computation();


        bool update(const QualifiedLabelRelation& label_relation);
        bool update_pair(int s, int t, const QualifiedLabelRelation& label_relation);
        bool relation_initialize(int s, int t, const QualifiedLabelRelation& label_relation);
        QualifiedLocalStateRelation2(const fts::LabelledTransitionSystem& lts, int factor, const fts::FTSTask& fts_task,
                                     const QualifiedLabelRelation& label_relation);



        [[nodiscard]] bool simulates(int t, int s) const {
            return simulations.at(t).contains(s);
        }

        [[nodiscard]] bool never_simulates(int t, int s) const {
            // t never simulates s if all transitions from s has no response from t
            return std::ranges::all_of(transition_responses[s][t], [](const auto& trs) {
                return trs.empty();
            });
        }

        [[nodiscard]] bool similar(int s, int t) const {
            return simulates(s, t) && simulates(t, s);
        }

        [[nodiscard]] int num_states() const {
            return lts.size();
        }

        bool is_identity() const;
        int num_equivalences() const;
        int num_simulations(bool ignore_equivalences) const;
        int num_different_states() const;


        void dump(utils::LogProxy &log, const std::vector<std::string> &names) const;
        void dump(utils::LogProxy &log) const;


        //Computes the probability of selecting a random pair s, s' such that s simulates s'.
        double get_percentage_simulations(bool ignore_equivalences) const;

        //Computes the probability of selecting a random pair s, s' such that s is equivalent to s'.
        double get_percentage_equivalences() const;

    };
}
#endif
