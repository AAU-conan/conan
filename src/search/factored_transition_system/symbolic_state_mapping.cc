#include "symbolic_state_mapping.h"
#include "factored_state_mapping.h"

namespace fts {
    void FactoredSymbolicStateMapping::construct(const FactoredStateMappingMergeAndShrink &mapping,
                                                      std::shared_ptr<symbolic::SymVariables> vars) {

        for(const auto & mas_for_factor : mapping.get_mapping()){
            mapping_by_factor.emplace_back(vars, *mas_for_factor);
        }
    }

    void FactoredSymbolicStateMapping::construct(const FactoredStateMappingIdentity & mapping, const std::shared_ptr<symbolic::SymVariables> vars) {

    }
}