#include "labelled_transition_system.h"

#include "fact_names.h"
#include "label_map.h"
#include "../abstract_task.h"
#include "../merge_and_shrink/transition_system.h"
#include "../merge_and_shrink/labels.h"

class AbstractTask;
using namespace std;

namespace fts {
    LabelledTransitionSystem::LabelledTransitionSystem(const merge_and_shrink::TransitionSystem &ts,
                                                       const LabelMap &labelMap,
                                                       FactValueNames fact_value_names) :
            num_states(ts.get_size()), goal_states(ts.get_goal_states()), init_state(ts.get_init_state()), fact_value_names(std::move(fact_value_names)), num_labels(0) {

        int num_label_groups = labelMap.get_num_labels();

        label_group_of_label.resize(num_label_groups, LabelGroup(-1));
        label_groups.reserve(num_label_groups);

        transitions_src.resize(num_states);
        transitions_label_group.reserve(num_label_groups);

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
                num_labels += new_label_group.size();
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

        for (const auto& [lg_i, _] : std::views::enumerate(label_groups)) {
            if (const auto lg = LabelGroup(static_cast<int>(lg_i)); !is_self_loop_everywhere_label(lg)) {
                relevant_label_groups.insert(lg);
            }
        }
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

    bool LabelledTransitionSystem::is_self_loop_everywhere_label(LabelGroup lg) const {
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


