#include "factored_state_mapping.h"

#include "../merge_and_shrink/merge_and_shrink_representation.h"
#include "../merge_and_shrink/types.h"

namespace fts {
    std::vector<int> FactoredStateMappingMergeAndShrink::transform(const std::vector<int> &state) {
        std::vector<int> result;
        result.reserve(factored_mapping.size());
        for (const auto & f : factored_mapping) {
            result.push_back(f->get_value(state));
        }

        return result;
    }


    std::optional<std::vector<int>>
    FactoredStateMappingMergeAndShrink::update_transformation_in_place(std::vector<int> &transformed_state_values,
                                                                       const std::vector<int> &state_values,
                                                                       std::vector<int> &updated_state_variables) {
        std::vector<int> updated_factors;
        for(int var : updated_state_variables) {
            updated_factors.push_back(variable_to_factor[var]);
        }
        std::ranges::sort(updated_factors);
        updated_factors.erase( std::unique(updated_factors.begin(), updated_factors.end()), updated_factors.end() );

        for (int factor : updated_factors) {
            transformed_state_values[factor] = factored_mapping[factor]->get_value(state_values);
            if (transformed_state_values[factor] == merge_and_shrink::PRUNED_STATE) {
                return std::nullopt;
            }
        }

        return {updated_factors};
    }

    int FactoredStateMappingMergeAndShrink::get_value(const std::vector<int> &state, int factor) {
        return factored_mapping[factor]->get_value(state);
    }

    // TODO (efficiency): Can we avoid copying the state here?
    std::vector<int> FactoredStateMappingIdentity::transform(const std::vector<int> &state) {
        return state;
    }

    std::optional<std::vector<int>>
    FactoredStateMappingIdentity::update_transformation_in_place(std::vector<int> &transformed_state_values,
                                                                 const std::vector<int> &state_values,
                                                                 std::vector<int> &updated_state_variables) {
        for (int var: updated_state_variables) {
            transformed_state_values[var] = state_values[var];
        }
        return {updated_state_variables};
    }

    int FactoredStateMappingIdentity::get_value(const std::vector<int> &state, int factor) {
        return state[factor];
    }
}