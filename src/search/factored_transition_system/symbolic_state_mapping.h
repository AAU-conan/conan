#ifndef FTS_SYMBOLIC_STATE_MAPPING_H
#define FTS_SYMBOLIC_STATE_MAPPING_H


#include "factored_state_mapping.h"
#include "../symbolic/sym_variables.h"

namespace merge_and_shrink {
    class MergeAndShrinkRepresentation;
}

namespace fts {
    class FactoredStateMappingIdentity;
    class FactoredStateMappingMergeAndShrink;

    class SymbolicStateMapping {
        std::shared_ptr<symbolic::SymVariables> vars;
        std::vector<BDD> value_bdd;

    public:
        SymbolicStateMapping(const std::shared_ptr<symbolic::SymVariables> &vars,
                             const merge_and_shrink::MergeAndShrinkRepresentation &state_mapping);

        SymbolicStateMapping(const std::shared_ptr<symbolic::SymVariables> &vars, int variable);

        BDD get_bdd(int value) const{
            return value_bdd[value];
        }

    };

    class FactoredSymbolicStateMapping {
        std::vector<SymbolicStateMapping> mapping_by_factor;

        void construct(const FactoredStateMappingMergeAndShrink & mapping, std::shared_ptr<symbolic::SymVariables> vars);
        void construct(const FactoredStateMappingIdentity & mapping, std::shared_ptr<symbolic::SymVariables> vars);

    public:

        FactoredSymbolicStateMapping(const FactoredStateMapping & mapping, std::shared_ptr<symbolic::SymVariables> vars) {
            if (dynamic_cast<const FactoredStateMappingMergeAndShrink *>(&mapping)) {
                construct(dynamic_cast<const FactoredStateMappingMergeAndShrink &>(mapping), vars);
            } else if (dynamic_cast<const FactoredStateMappingIdentity *>(&mapping)) {
                construct(dynamic_cast<const FactoredStateMappingIdentity &>(mapping), vars);
            } else {
                std::cerr << "Unsupported FactoredStateMapping type" << std::endl;
                utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
            }
        }

        const SymbolicStateMapping & operator [] (int factor) const {
            return mapping_by_factor[factor];
        }

        size_t size() const {
            return mapping_by_factor.size();
        }
    };
}
#endif
