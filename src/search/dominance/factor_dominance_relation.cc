#include "factor_dominance_relation.h"

#include "../factored_transition_system/labelled_transition_system.h"
#include "../utils/logging.h"
#include "../plugins/plugin.h"

using namespace std;
using fts::LabelledTransitionSystem;

namespace dominance {
    FactorDominanceRelation::FactorDominanceRelation(int num_states) : num_states(num_states) {}

    [[nodiscard]] bool FactorDominanceRelation::is_identity() const {
        for (int i = 0; i < num_states; ++i) {
            for (int j = i + 1; j < num_states; ++j) {
                if (simulates(i, j) || simulates(j, i)) {
                    return false;
                }
            }
        }
        return true;
    }

    void FactorDominanceRelation::dump(utils::LogProxy& log, const LabelledTransitionSystem& lts) const {
        log << "SIMREL:" << std::endl;
        for (int j = 0; j < num_states; ++j) {
            for (int i = 0; i < num_states; ++i) {
                if (simulates(j, i) && i != j) {
                    if (simulates(i, j)) {
                        if (j < i) {
                            log << lts.state_name(i) << " <=> " << lts.state_name(j) << std::endl;
                        }
                    } else {
                        log << lts.state_name(i) << " <= " << lts.state_name(j) << std::endl;
                    }
                }
            }
        }
    }

    int FactorDominanceRelation::num_equivalences() const {
        int num = 0;
        for (int i = 0; i < static_cast<int>(num_states); i++) {
            for (int j = i + 1; j < static_cast<int>(num_states); j++) {
                if (similar(i, j)) {
                    num++;
                }
            }
        }
        return num;
    }

    int FactorDominanceRelation::num_simulations() const {
        int res = 0;
        std::vector<bool> counted(num_states, false);
        for (int i = 0; i < num_states; ++i) {
            if (!counted[i]) {
                for (int j = i + 1; j < num_states; ++j) {
                    if (similar(i, j)) {
                        counted[j] = true;
                    }
                }
            }
        }
        for (int i = 0; i < num_states; ++i) {
            if (!counted[i]) {
                for (int j = i + 1; j < num_states; ++j) {
                    if (!counted[j]) {
                        if (!similar(i, j) && (simulates(i, j) || simulates(j, i))) {
                            res++;
                        }
                    }
                }
            }
        }
        return res;
    }

    int FactorDominanceRelation::num_different_states() const {
        int num = 0;
        std::vector<bool> counted(num_states, false);
        for (int i = 0; i < static_cast<int>(counted.size()); i++) {
            if (!counted[i]) {
                num++;
                for (int j = i + 1; j < num_states; j++) {
                    if (similar(i, j)) {
                        counted[j] = true;
                    }
                }
            }
        }
        return num;
    }



    static class FactorDominanceRelationFactoryPlugin final : public plugins::TypedCategoryPlugin<FactorDominanceRelationFactory> {
    public:
        FactorDominanceRelationFactoryPlugin() : TypedCategoryPlugin("FactorDominanceRelationFactory") {
            document_synopsis( "A FactorDominanceRelationFactory creates FactorDominanceRelations for a given LTS.");
        }
    }
    _category_plugin;



}
