#include "label_map.h"
#include "../merge_and_shrink/labels.h"

#include <iostream>
namespace fts {

    LabelMap::LabelMap(const merge_and_shrink::Labels & labels) {
        //TODO: Ensure that all dead labels have been removed (marked as inactive)
        num_valid_labels = 0;
        label_id.reserve(labels.get_num_total_labels());
        old_label_id.reserve(labels.get_num_active_labels());
        for (int i = 0; i < labels.get_num_total_labels(); i++) {
            if (!labels.is_active(i)) {
                label_id.push_back(-1);
            } else {
                old_label_id.push_back(i);
                label_id.push_back(num_valid_labels++);
            }
        }
    }
}