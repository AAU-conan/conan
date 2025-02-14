#include "symbolic_state_mapping.h"
#include "factored_state_mapping.h"

namespace fts {
    SymbolicStateMapping::SymbolicStateMapping(const std::shared_ptr<symbolic::SymVariables> &,
        const merge_and_shrink::MergeAndShrinkRepresentation &) {
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    SymbolicStateMapping::SymbolicStateMapping(const std::shared_ptr<symbolic::SymVariables> &vars, int variable) : vars (vars) {
        for (size_t val = 0; val < vars->get_domain_size(variable); ++val){
            value_bdd.push_back(vars->preBDD(variable, val));
        }
    }

    void FactoredSymbolicStateMapping::construct(const FactoredStateMappingMergeAndShrink &mapping,
                                                 std::shared_ptr<symbolic::SymVariables> vars) {

        for(const auto & mas_for_factor : mapping.get_mapping()){
            mapping_by_factor.emplace_back(vars, *mas_for_factor);
        }
    }

    void FactoredSymbolicStateMapping::construct(const FactoredStateMappingIdentity & mapping, const std::shared_ptr<symbolic::SymVariables> vars) {
        utils::unused_variable(mapping);
        for (size_t v= 0; v < vars->get_num_variables(); ++v){
            mapping_by_factor.emplace_back(vars, v);
        }
    }

}