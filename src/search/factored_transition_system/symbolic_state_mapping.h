#ifndef FTS_SYMBOLIC_STATE_MAPPING_H
#define FTS_SYMBOLIC_STATE_MAPPING_H


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
    public:
        FactoredSymbolicStateMapping(const FactoredStateMappingMergeAndShrink & mapping, std::shared_ptr<symbolic::SymVariables> vars);
        FactoredSymbolicStateMapping(const FactoredStateMappingIdentity & mapping, std::shared_ptr<symbolic::SymVariables> vars);
    };
}
#endif
