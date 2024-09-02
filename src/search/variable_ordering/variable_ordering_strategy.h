#ifndef VARIABLE_ORDERING_VARIABLE_ORDERING_STRATEGY_H
#define VARIABLE_ORDERING_VARIABLE_ORDERING_STRATEGY_H

#include <vector>

class AbstractTask;

namespace variable_ordering {
    class VariableOrderingStrategy {
    public:
        virtual ~VariableOrderingStrategy(){}
        virtual std::vector<int> compute_variable_ordering(const AbstractTask & task) const = 0;
        virtual void print_options() const = 0;

    };
}
#endif
