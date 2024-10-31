#ifndef QUALIFIED_LABEL_RELATION_H
#define QUALIFIED_LABEL_RELATION_H

#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <memory>
#include <generator>

#include "../dominance/all_none_factor_index.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "label_group_relation.h"


using namespace dominance;

namespace fts {
    class FTSTask;
    class LabelledTransitionSystem;
    class LabelMap;
}

namespace utils {
    class LogProxy;
}

namespace qdominance {
    using TransitionIndex = size_t;
    class QualifiedLocalStateRelation2;

    /*
     * Label relation represents the preorder relations on labels that
     * occur in a set of LTS
     */
    class QualifiedLabelRelation {
        const fts::FTSTask &fts_task;

        std::vector<LabelGroupSimulationRelation> label_group_simulation_relations;

        // For each label l1, a map from label to the factors in which l1 dominates l2. If l2 is not present, it dominates in none.
        std::vector<std::unordered_map<int, AllNoneFactorIndex> > dominates_in;

        // For each factor i, label group lg, state s, the states s' such that s -lg-> s'
        std::vector<std::vector<std::vector<std::vector<int>>>> label_state_transition_map;

        [[nodiscard]] const std::vector<int>& targets_for_label_group_state(int factor, fts::LabelGroup lg, int s) const {
            return label_state_transition_map[factor][lg.group][s];
        }

        // For factor i, the label groups which are simulated by/simulates irrelevant labels
        std::vector<std::set<fts::LabelGroup>> simulated_by_irrelevant;
        std::vector<std::set<fts::LabelGroup>> simulates_irrelevant;

        std::vector<AllNoneFactorIndex> dominated_by_noop_in;


        void set_not_simulates(int l1, int l2, int factor);

        void set_not_simulated_by_noop(int l, int factor);

        int necessary_removals = 0;
        int unnecessary_removals = 0;

    public:
        explicit QualifiedLabelRelation(const fts::FTSTask& fts_task);

        bool update(std::vector<std::unique_ptr<QualifiedLocalStateRelation2>>& sims);
        bool update_label_pair(int l1, int l2, int factor,
                               const QualifiedLocalStateRelation2& sim);
        bool update_irrelevant(int factor, const QualifiedLocalStateRelation2& sim);

        void dump(utils::LogProxy& log) const;
        void dump(utils::LogProxy& log, int label) const;
        void dump_dominance(utils::LogProxy& log) const;

        [[nodiscard]] bool label_group_simulates(int factor, fts::LabelGroup lg1, fts::LabelGroup lg2) const;
        [[nodiscard]] bool label_group_simulated_by_noop(int factor, fts::LabelGroup lg) const;


        [[nodiscard]] int get_num_labels() const {
            return fts_task.get_num_labels();
        }

        [[nodiscard]] AllNoneFactorIndex get_dominated_by_noop_in(int l) const {
            return dominated_by_noop_in[l];
        }

        [[nodiscard]] bool dominated_by_noop(int l, int lts) const {
            return dominated_by_noop_in[l].contains_all_except(lts);
        }

        //Returns true if l1 simulates l2 in lts
        [[nodiscard]] bool simulates(int l1, int l2, int factor) const {
            const auto& l1_dominates_in = dominates_in.at(l1);
            return l1_dominates_in.contains(l2)? l1_dominates_in.at(l2).contains(factor) : false;
        }
    };
}


#endif
