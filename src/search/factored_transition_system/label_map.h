#ifndef DOMINANCE_LABEL_MAP_H
#define DOMINANCE_LABEL_MAP_H

#include "../operator_cost.h"

#include <vector>
#include <set>
#include <list>
#include <optional>

namespace  merge_and_shrink {
    class Labels;
}

namespace fts {

    class LabelMap {
        //mapping from labels to labels for LTSs (hack to get rid of not useful labels)
        int num_valid_labels;
        std::vector<int> label_id;
        std::vector<int> old_label_id;
    public:
        LabelMap(const merge_and_shrink::Labels & labels);

        std::optional<int> get_id(int i) const {
            if(label_id[i] == -1) {
                return std::nullopt;
            }
            return label_id[i];
        }

        int get_old_id(int i) const {
            return old_label_id[i];
        }

        int get_num_labels() const {
            return num_valid_labels;
        }

    };
}
#endif
