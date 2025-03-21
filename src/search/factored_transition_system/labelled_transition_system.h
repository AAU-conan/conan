
#ifndef MERGE_AND_SHRINK_LABELLED_TRANSITION_SYSTEM_H
#define MERGE_AND_SHRINK_LABELLED_TRANSITION_SYSTEM_H

#include <functional>
#include <vector>
#include <string>
#include <algorithm>    // std::find
#include <cassert>
#include <generator>
#include <ranges>
#include <memory>

class AbstractTask;

namespace merge_and_shrink {
    class TransitionSystem;
}

namespace fts {
    class FactValueNames;
    typedef int AbstractStateRef;

    class LabelMap;

    class LabelGroup {
    public:
        int group;

        explicit LabelGroup(int g) : group(g) {
        }

        bool operator==(const LabelGroup &other) const {
            return group == other.group;
        }

        bool operator!=(const LabelGroup &other) const {
            return !(*this == other);
        }

        LabelGroup &operator++() {
            group++;
            return *this;
        }

        bool operator<(const LabelGroup &other) const {
            return group < other.group;
        }

        bool is_dead() const {
            return group == -1;
        }
    };
}



namespace fts {
    class LTSTransition {
    public:
        AbstractStateRef src, target;
        LabelGroup label_group;

        LTSTransition(AbstractStateRef _src, AbstractStateRef _target, LabelGroup _label) :
                src(_src), target(_target), label_group(_label) {
        }

        bool operator==(const LTSTransition &other) const {
            return src == other.src && target == other.target && label_group == other.label_group;
        }

        bool operator!=(const LTSTransition &other) const {
            return !(*this == other);
        }

        bool operator<(const LTSTransition &other) const {
            return src < other.src || (src == other.src && target < other.target)
                   || (src == other.src && target == other.target && label_group < other.label_group);
        }

        bool operator>=(const LTSTransition &other) const {
            return !(*this < other);
        }
    };

    struct TSTransition {
        AbstractStateRef src, target;

        TSTransition(AbstractStateRef _src, AbstractStateRef _target) :
                src(_src), target(_target) {
        }

        bool operator==(const TSTransition &other) const {
            return src == other.src && target == other.target;
        }

        bool operator!=(const TSTransition &other) const {
            return !(*this == other);
        }

        bool operator<(const TSTransition &other) const {
            return src < other.src || (src == other.src && target < other.target);
        }

        bool operator>=(const TSTransition &other) const {
            return !(*this < other);
        }
    };


    // This is very similar to merge_and_shrink::TransitionSystem
    // An important detail is that this is designed for being inmutable, not have any innactive states/labels,
    // and has redundant representation to allow for faster access to diverse sets of transitions
    //
    // The design of label groups is slightly different to that of merge_and_shrink::TransitionSystem. The reason is
    // historic, this was implemented in an older version of FastDownward where merge_and_shrink::TransitionSystem
    // didn't have label groups.
    // It would be nice to have an implementation that is closer to that of merge_and_shrink::TransitionSystem and allows for
    // moving the representations.
    class LabelledTransitionSystem {
        //Duplicated from abstraction
        int num_states;
        int num_labels;
        std::vector<bool> goal_states;
        AbstractStateRef init_state;
        std::vector<LabelGroup> relevant_label_groups;
        std::vector<bool> label_group_is_relevant;
        std::vector<int> goal_distances; // TODO: Possibly unify with merge_and_shrink::Distances


        std::vector<std::vector<int> > label_groups;
        std::vector<LabelGroup> label_group_of_label; //TODO: Make a map to avoid representing irrelevant labels explicitly?
        std::vector<LTSTransition> transitions;
        std::vector<std::vector<LTSTransition> > transitions_src;
        std::vector<std::vector<TSTransition> > transitions_label_group;


        bool is_self_loop_everywhere_label_group(LabelGroup lg) const;

    public:
        std::shared_ptr<FactValueNames> fact_value_names;
        LabelledTransitionSystem(const merge_and_shrink::TransitionSystem &abs, const LabelMap &labelMap, std::shared_ptr<FactValueNames> fact_value_names);

        ~LabelledTransitionSystem() {}

        const std::vector<bool> &get_goal_states() const {
            return goal_states;
        }

        const std::vector<int> get_goal_distances () const {
            return goal_distances;
        }

        bool is_goal(int state) const {
            return goal_states.at(state);
        }

        inline int size() const {
            return num_states;
        }

        int num_transitions() const {
            return transitions.size();
        }

        const std::vector<LTSTransition> &get_transitions() const {
            return transitions;
        }

        const std::vector<LTSTransition> &get_transitions(int sfrom) const {
            return transitions_src[sfrom];
        }

        const std::vector<TSTransition> &get_transitions_label(int label) const {
            return transitions_label_group[label_group_of_label[label].group];
        }

        const std::vector<TSTransition> &get_transitions_label_group(LabelGroup label_group) const {
            return transitions_label_group[label_group.group];
        }

        [[nodiscard]] std::string state_name(int s) const;

        [[nodiscard]] std::string label_name(int label) const;

        [[nodiscard]] std::string label_group_name(const LabelGroup& lg) const;

        bool is_relevant_label(int label) const {
            return is_relevant_label_group(label_group_of_label.at(label));
        }

        bool is_relevant_label_group(const LabelGroup &label_group) const {
            return label_group_is_relevant.at(label_group.group);
        }

        const std::vector<LabelGroup>& get_relevant_label_groups() const {
            return relevant_label_groups;
        }

        AbstractStateRef get_initial_state() const {
            return init_state;
        }

        //For each transition labelled with l, applya a function. If returns true, applies a break
        bool applyPostSrc(int from,
                          std::function<bool(const LTSTransition &tr)> &&f) const {
            for (const auto &tr: transitions_src[from]) {
                if (f(tr)) return true;
            }
            return false;
        }

        [[nodiscard]] const std::vector<std::vector<int>>& get_label_groups() const {
            return label_groups;
        }

        const std::vector<int> &get_labels(LabelGroup label_group) const {
            return label_groups[label_group.group];
        }

        LabelGroup get_group_label(int label) const {
            return LabelGroup(label_group_of_label[label]);
        }

        int get_num_label_groups() const {
            return label_groups.size();
        }

        int get_num_labels() const {
            return num_labels;
        }

        const std::vector<LabelGroup> &get_group_of_label() const {
            return label_group_of_label;
        }

        void dump() const;
        bool irrelevant_label_group(LabelGroup lg) const;


        auto get_irrelevant_labels() const {
            auto all_numbers = std::views::iota(0) | std::views::take(label_group_of_label.size());
            return all_numbers | std::views::filter([&](int label) {
                return !is_relevant_label(label);
            });

        }
    };
}


#endif
