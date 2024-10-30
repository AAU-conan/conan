#ifndef FACT_NAMES_H
#define FACT_NAMES_H
#include <memory>
#include <string>
#include <map>
#include <ranges>
#include <set>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

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
    FactValueNames(const FactValueNames& other)
        : fact_names(other.fact_names->clone()), variable(other.variable)
    {}

    FactValueNames(const FactNames& fact_names, int variable)
     : fact_names(fact_names.clone()), variable(variable)
    { }

    [[nodiscard]] std::string get_fact_value_name(const int value) const {
        return fact_names->get_fact_name(FactPair(variable, value));
    }

    [[nodiscard]] std::string get_operator_name(const int index, const bool is_axiom) const {
        return fact_names->get_operator_name(index, is_axiom);
    }

    [[nodiscard]] std::string get_common_operators_name(const std::vector<int>& ops) const {
        // operator name is space separated, for each word position store the set of strings that are at that position
        std::map<long, std::set<std::string>> op_index_to_names;

        for (const auto& op : ops) {
            auto op_name = fact_names->get_operator_name(op, false);
            std::vector<std::string> op_name_vec;
            boost::split(op_name_vec, op_name, boost::is_any_of(" "));
            for (const auto& [i, word] : std::views::enumerate(op_name_vec)) {
                op_index_to_names[i].insert(word);
            }
        }

        std::string result;
        for (long i = 0; op_index_to_names.contains(i); ++i) {
            if (i == 0) {
                result += boost::join(op_index_to_names[i], "|");
            } else {
                result += " " + (op_index_to_names[i].size() == 1? *op_index_to_names[i].begin(): "_");
            }
        }
        return result;
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
