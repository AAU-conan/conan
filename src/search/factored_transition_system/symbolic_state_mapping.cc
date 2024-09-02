#include "symbolic_state_mapping.h"
#include "factored_state_mapping.h"

fts::FactoredSymbolicStateMapping::FactoredSymbolicStateMapping(const fts::FactoredStateMappingMergeAndShrink &mapping,
                                                                std::shared_ptr<symbolic::SymVariables> vars) {

    for(const auto & mas_for_factor : mapping.get_mapping()){
        mapping_by_factor.emplace_back(vars, *mas_for_factor);
    }
}

fts::SymbolicStateMapping::SymbolicStateMapping(const std::shared_ptr<symbolic::SymVariables> &vars,
                                                const merge_and_shrink::MergeAndShrinkRepresentation &state_mapping) {

}
