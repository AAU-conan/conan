#include "fact_names.h"

#include "../abstract_task.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

namespace fts {
    NoFactNames::NoFactNames() = default;

    std::string NoFactNames::get_operator_name(int index, bool is_axiom) const {
        return std::format("Operator-{}-{}", index, is_axiom);
    }

    std::string NoFactNames::get_variable_name(int variable) const {
        return std::format("Var-{}", variable);
    }

    std::string NoFactNames::get_fact_name(const FactPair& fact_pair) const {
        return std::format("Fact-{}-{}", fact_pair.var, fact_pair.value);
    }

    std::unique_ptr<FactNames> NoFactNames::clone() const {
        return std::make_unique<NoFactNames>(*this);
    }

    int NoFactNames::get_num_operators() const {
        throw std::logic_error("NoFactNames cannot be used to get the number of operators");
    }

    size_t NoFactNames::get_num_variables() const {
        throw std::logic_error("NoFactNames cannot be used to get the number of variables");
    }

    std::string AbstractTaskFactNames::get_operator_name(const int index, const bool is_axiom) const {
        return abstract_task->get_operator_name(index, is_axiom);
    }

    std::string AbstractTaskFactNames::get_variable_name(const int variable) const {
        return abstract_task->get_variable_name(variable);
    }

    std::string AbstractTaskFactNames::get_fact_name(const FactPair& fact_pair) const {
        return abstract_task->get_fact_name(fact_pair);
    }

    std::unique_ptr<FactNames> AbstractTaskFactNames::clone() const {
        return std::make_unique<AbstractTaskFactNames>(*this);
    }

    int AbstractTaskFactNames::get_num_operators() const {
        return abstract_task->get_num_operators();
    }

    size_t AbstractTaskFactNames::get_num_variables() const {
        return abstract_task->get_num_variables();
    }

    FactValueNames::FactValueNames(const std::shared_ptr<FactNames>& fact_names, int variable): fact_names(fact_names), variable(variable) { }

    std::string FactValueNames::get_fact_value_name(const int value) const {
        return fact_names->get_fact_name(FactPair(variable, value));
    }

    std::string FactValueNames::get_operator_name(const int index, const bool is_axiom) const {
        return fact_names->get_operator_name(index, is_axiom);
    }

    std::string FactValueNames::get_common_operators_name(const std::vector<int>& ops) const {
        return boost::join(std::ranges::to<std::vector<std::string>>(std::views::transform(ops, [&](int op) { return get_operator_name(op, false); })), " | ");
    }

    std::unique_ptr<FactValueNames> get_debug_or_release_fact_value_names(const std::shared_ptr<FactNames>& fact_names, int variable) {
#ifndef NDEBUG
        return std::make_unique<NfaAggregatableFactValueNames>(fact_names, variable);
#else
        return std::make_unique<FactValueNames>(fact_names, variable);
#endif
    }

    NfaAggregatableFactValueNames::NfaAggregatableFactValueNames(const std::shared_ptr<FactNames>& fact_names,
                                                                 int variable): FactValueNames(fact_names, variable) {
        mata::Symbol next_symbol = 0;
        auto initial_state = full_nfa.add_state();
        full_nfa.initial.insert(initial_state);

        std::vector<mata::Symbol> symbols;
        for (int op = 0; op < fact_names->get_num_operators(); ++op) {
            auto op_name = get_operator_name(op, false);
            std::vector<std::string> op_name_vec;
            boost::split(op_name_vec, op_name, boost::is_any_of(" "));
            mata::nfa::State current_state = initial_state;
            for (const auto& name_i : op_name_vec) {
                if (symbol_to_word.right.find(name_i) == symbol_to_word.right.end()) {
                    symbols.push_back(next_symbol);
                    symbol_to_word.insert({next_symbol++, name_i});
                }
                auto next_state = full_nfa.add_state();
                full_nfa.delta.add(current_state, symbol_to_word.right.at(name_i), next_state);
                current_state = next_state;
            }
            full_nfa.final.insert(current_state);
        }
        alphabet = new mata::EnumAlphabet(symbols.begin(), symbols.end());
        full_nfa.alphabet = alphabet;
        full_nfa = minimize(full_nfa);
        full_nfa.alphabet = alphabet;
    }

    std::string NfaAggregatableFactValueNames::get_fact_value_name(const int value) const {
        return fact_names->get_fact_name(FactPair(variable, value));
    }

    std::string NfaAggregatableFactValueNames::get_operator_name(const int index, const bool is_axiom) const {
        return fact_names->get_operator_name(index, is_axiom);
    }

    std::string NfaAggregatableFactValueNames::get_common_operators_name(const std::vector<int>& ops) const {
        mata::nfa::Nfa nfa{};
        nfa.alphabet = alphabet;
        auto initial_state = nfa.add_state();
        nfa.initial.insert(initial_state);

        for (const auto& op : ops) {
            auto op_name = get_operator_name(op, false);
            std::vector<std::string> op_name_vec;
            boost::split(op_name_vec, op_name, boost::is_any_of(" "));
            auto current_state = initial_state;
            for (const auto & name_i : op_name_vec) {
                auto next_state = nfa.add_state();
                nfa.delta.add(current_state, symbol_to_word.right.at(name_i), next_state);
                current_state = next_state;
            }
            nfa.final.insert(current_state);
        }
        nfa.alphabet = alphabet;
        nfa = minimize(nfa);
        nfa.alphabet = alphabet;

        return boost::join(create_common_operator_names(nfa, initial_state, mata::nfa::StateSet{full_nfa.initial}), "\n");
    }

    /*
     * For an NFA representing the set of operator names, and an NFA representing the set of all possible operator names,
     * constructs a sequence of compact operator names, that distinguishes the operators from all possible operators.
     */
    std::vector<std::string> NfaAggregatableFactValueNames::create_common_operator_names(const mata::nfa::Nfa& op_nfa,
        mata::nfa::State op_state, const mata::nfa::StateSet& full_states) const {
        if (op_nfa.final.contains(op_state)) {
            return {""};
        }

        std::unordered_map<mata::nfa::State, std::set<mata::Symbol>> next_transitions;

        for (const auto& symbol_post : op_nfa.delta.state_post(op_state)) {
            for (const auto& next_state : symbol_post.targets) {
                next_transitions[next_state].insert(symbol_post.symbol);
            }
        }
        std::vector<std::string> result;
        auto full_nfa_copy = mata::nfa::Nfa(full_nfa);
        full_nfa_copy.initial = mata::utils::SparseSet<mata::nfa::State>(full_states.begin(), full_states.end());
        auto op_nfa_copy = mata::nfa::Nfa(op_nfa);
        op_nfa_copy.initial = {op_state};

        bool is_wildcard = are_equivalent(full_nfa_copy, op_nfa_copy, alphabet);

        for (const auto& [next_state, symbols] : next_transitions) {
            std::string symbols_name;
            if (is_wildcard) {
                symbols_name = "_";
            } else {
                symbols_name = boost::join(std::views::transform(symbols, [&](mata::Symbol s) { return symbol_to_word.left.at(s); }) | std::ranges::to<std::vector<std::string>>(), "|");
            }

            mata::nfa::StateSet next_full_states;
            for (const auto& s : symbols) {
                for (const auto& nfs : full_nfa.post(full_states, s)) {
                    next_full_states.insert(nfs);
                }
            }

            for (const auto& rest : create_common_operator_names(op_nfa, next_state, next_full_states)) {
                result.push_back(std::format("{} {}", symbols_name, rest));
            }

            if (is_wildcard) // As it is, wildcards will always make all the remaining wildcards, so only do one
                break;
        }
        return result;
    }
} // fts