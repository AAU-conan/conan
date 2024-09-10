#ifndef FACT_NAMES_H
#define FACT_NAMES_H
#include <memory>
#include <string>
#include "../abstract_task.h"

namespace fts {

class FactNames {
public:
    virtual ~FactNames() = default;
    [[nodiscard]] virtual std::string get_operator_name(int index, bool is_axiom) const = 0;
    [[nodiscard]] virtual std::string get_variable_name(int var) const = 0;
    [[nodiscard]] virtual std::string get_fact_name(const FactPair& fact_pair) const = 0;
    [[nodiscard]] virtual std::unique_ptr<FactNames> clone() const = 0;
};

class FactValueNames {
protected:
    std::unique_ptr<FactNames> fact_names;
    int variable;
public:
    FactValueNames(const FactNames& fact_names, int variable)
     : fact_names(fact_names.clone()), variable(variable)
    { }

    [[nodiscard]] std::string get_fact_value_name(const int value) const {
        return fact_names->get_fact_name(FactPair(variable, value));
    }
};


class AbstractTaskFactNames final : public FactNames {
    std::shared_ptr<AbstractTask> abstract_task;

public:
    AbstractTaskFactNames() = default;
    explicit AbstractTaskFactNames(const std::shared_ptr<AbstractTask>& abstract_task)
        : abstract_task(abstract_task) {
    }

    [[nodiscard]] std::string get_operator_name(const int index, const bool is_axiom) const override {
        return abstract_task->get_operator_name(index, is_axiom);
    }

    [[nodiscard]] std::string get_variable_name(const int variable) const override {
        return abstract_task->get_variable_name(variable);
    }

    [[nodiscard]] std::string get_fact_name(const FactPair& fact_pair) const override {
        return abstract_task->get_fact_name(fact_pair);
    }

    [[nodiscard]] std::unique_ptr<FactNames> clone() const override {
        return std::make_unique<AbstractTaskFactNames>(*this);
    }
};

class NoFactNames final : public FactNames {
public:
    NoFactNames() = default;

    [[nodiscard]] std::string get_operator_name(int index, bool is_axiom) const override {
        return std::format("Operator-{}-{}", index, is_axiom);
    }

    [[nodiscard]] std::string get_variable_name(int variable) const override {
        return std::format("Var-{}", variable);
    }

    [[nodiscard]] std::string get_fact_name(const FactPair& fact_pair) const override {
        return std::format("Fact-{}-{}", fact_pair.var, fact_pair.value);
    }

    [[nodiscard]] std::unique_ptr<FactNames> clone() const override {
        return std::make_unique<NoFactNames>(*this);
    }
};

} // fts

#endif //FACT_NAMES_H
