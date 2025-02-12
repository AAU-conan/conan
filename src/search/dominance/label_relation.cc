#include "label_relation.h"

#include "factor_dominance_relation.h"
#include "state_dominance_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../utils/logging.h"
#include "../merge_and_shrink/labels.h"
#include "../factored_transition_system/label_map.h"
#include "../plugins/plugin.h"

using namespace std;
using namespace fts;

namespace dominance {
    LabelRelation::LabelRelation(int num_labels) : num_labels(num_labels) { }

    void LabelRelation::dump(utils::LogProxy& log, const fts::FTSTask& fts_task ) const {
        for (int i = 0; i < static_cast<int>(fts_task.get_factors().size()); ++i) {
            log << std::format("Factor {}", i) << std::endl;
            for (int l1 = 0; l1 < num_labels; ++l1) {
                for (int l2 = 0; l2 < num_labels; ++l2) {
                    if (l1 != l2 && label_dominates_label_in_all_other(i, fts_task, l2, l1)) {
                        log << std::format("Label {} dominates {} in all other", fts_task.get_factor(i).label_name(l2), fts_task.get_factor(i).label_name(l1)) << std::endl;
                    }
                }
                if (noop_simulates_label_in_all_other(i, fts_task, l1)) {
                    log << std::format("NOOP dominates {} in all other", fts_task.get_factor(i).label_name(l1)) << std::endl;
                }
            }
        }
    }


    static class LabelRelationFactoryPlugin final : public plugins::TypedCategoryPlugin<LabelRelationFactory> {
    public:
        LabelRelationFactoryPlugin() : TypedCategoryPlugin("LabelRelationFactory") {
            document_synopsis( "A LabelRelationFactory creates LabelRelations for a given FTS task.");
        }
    }
    _category_plugin;
}