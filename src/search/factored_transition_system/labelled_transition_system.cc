#include "labelled_transition_system.h"

#include <utility>

#include "fact_names.h"
#include "label_map.h"
#include "../merge_and_shrink/transition_system.h"

class AbstractTask;
using namespace std;

namespace fts {
    LabelledTransitionSystem::LabelledTransitionSystem(const merge_and_shrink::TransitionSystem &ts,
                                                       const LabelMap &labelMap,
                                                       std::shared_ptr<FactValueNames> fact_value_names) :
            num_states(ts.get_size()), num_labels(labelMap.get_num_labels()), goal_states(ts.get_goal_states()), init_state(ts.get_init_state()), fact_value_names(std::move(fact_value_names)) {

        label_group_of_label.resize(num_labels, LabelGroup(-1));

        transitions_src.resize(num_states);

        for (const auto & local_label_info : ts) {
            const auto & abs_tr = local_label_info.get_transitions();

            if (!abs_tr.empty()) {
                LabelGroup new_label_group_id {(int)label_groups.size()};

                std::vector<int> new_label_group;
                for (int label : local_label_info.get_label_group()) {
                    auto maybe_new_label_id = labelMap.get_id(label);
                    if(maybe_new_label_id.has_value()) {
                        int new_label_id = maybe_new_label_id.value();
                        new_label_group.push_back(new_label_id);
                        label_group_of_label[new_label_id] = new_label_group_id;
                    }
                }
                assert(!new_label_group.empty());
                label_groups.push_back(new_label_group);
                transitions_label_group.push_back({});
                for (const auto & tr : abs_tr) {
                    transitions_label_group[new_label_group_id.group].push_back(TSTransition(tr.src, tr.target));
                    transitions.push_back(LTSTransition(tr.src, tr.target, new_label_group_id));
                    transitions_src[tr.src].push_back(LTSTransition(tr.src, tr.target, new_label_group_id));
                }
            } else {
                // Dead labels should have been removed
                assert(false);
            }
        }

        label_group_is_relevant.resize(label_groups.size(), false);
        for (const auto& [lg_i, _] : std::views::enumerate(label_groups)) {
            if (const auto lg = LabelGroup(static_cast<int>(lg_i)); !irrelevant_label_group(lg)) {
                relevant_label_groups.push_back(lg);
                label_group_is_relevant[lg_i] = true;
            }
        }
    }

    std::string LabelledTransitionSystem::state_name(int s) const {
        return fact_value_names->get_fact_value_name(s);
    }

    std::string LabelledTransitionSystem::label_name(int label) const {
        return fact_value_names->get_operator_name(label, false);
    }

    std::string LabelledTransitionSystem::label_group_name(const LabelGroup& lg) const {
        return fact_value_names->get_common_operators_name(get_labels(lg));
    }

    void LabelledTransitionSystem::dump() const {
        for (int s = 0; s < size(); s++) {
            applyPostSrc(s, [&](const LTSTransition &trs) {
                cout << trs.src << " -> " << trs.target << " (" << trs.label_group.group << ":";
                for (int tr_s_label: get_labels(trs.label_group)) {
                    cout << " " << tr_s_label;
                }
                cout << ")\n";
                return false;
            });
        }

    }

    /*
     * Returns true if the label group has a self-loop in every state and no other transitions
     */
    bool LabelledTransitionSystem::irrelevant_label_group(LabelGroup lg) const {
        const auto &trs = get_transitions_label_group(lg);
        return trs.size() == (size_t) num_states && is_self_loop_everywhere_label_group(lg);
    }

    /*
     * Returns true if the label group has a self loop in every state
     */
    bool LabelledTransitionSystem::is_self_loop_everywhere_label_group(LabelGroup lg) const {
        const auto &trs = get_transitions_label_group(lg);
        if (trs.size() < (size_t) num_states) return false;

        // This assumes that there is no repeated transition
        int num_self_loops = 0;
        for (const auto &tr: trs) {
            if (tr.src == tr.target) {
                num_self_loops++;
            }
        }

        assert(num_self_loops <= num_states);
        return num_self_loops == num_states;
    }

}


