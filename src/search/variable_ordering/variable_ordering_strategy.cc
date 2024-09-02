#include "variable_ordering_strategy.h"

#include "../plugins/plugin.h"


static class VariableOrderingCategoryPlugin : public plugins::TypedCategoryPlugin<variable_ordering::VariableOrderingStrategy> {
public:
    VariableOrderingCategoryPlugin() : TypedCategoryPlugin("VariableOrdering") {
        document_synopsis("Variable ordering strategy");
    }
}  _category_plugin;
