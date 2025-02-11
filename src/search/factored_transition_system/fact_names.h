#ifndef FTS_FACT_NAMES_H
#define FTS_FACT_NAMES_H
#include <memory>
#include <string>
#include <boost/bimap.hpp>
#include <mata/nfa/nfa.hh>

class AbstractTask;
class FactPair;

namespace fts {

class FactNames {
public:
    virtual ~FactNames() = default;
    [[nodiscard]] virtual std::string get_operator_name(int index, bool is_axiom) const = 0;
    [[nodiscard]] virtual std::string get_variable_name(int var) const = 0;
    [[nodiscard]] virtual std::string get_fact_name(const FactPair& fact_pair) const = 0;
    [[nodiscard]] virtual std::unique_ptr<FactNames> clone() const = 0;
    [[nodiscard]] virtual int get_num_operators() const = 0;
    [[nodiscard]] virtual size_t get_num_variables() const = 0;
};

    class NoFactNames final : public FactNames {
    public:
        NoFactNames();

        [[nodiscard]] std::string get_operator_name(int index, bool is_axiom) const override;

        [[nodiscard]] std::string get_variable_name(int variable) const override;

        [[nodiscard]] std::string get_fact_name(const FactPair& fact_pair) const override;

        [[nodiscard]] std::unique_ptr<FactNames> clone() const override;

        [[nodiscard]] int get_num_operators() const override;

        [[nodiscard]] size_t get_num_variables() const override;
    };


    class AbstractTaskFactNames final : public FactNames {
        std::shared_ptr<AbstractTask> abstract_task;

    public:
        AbstractTaskFactNames() = default;
        explicit AbstractTaskFactNames(const std::shared_ptr<AbstractTask>& abstract_task)
            : abstract_task(abstract_task) {


        }

        [[nodiscard]] std::string get_operator_name(const int index, const bool is_axiom) const override;

        [[nodiscard]] std::string get_variable_name(const int variable) const override;

        [[nodiscard]] std::string get_fact_name(const FactPair& fact_pair) const override;

        [[nodiscard]] std::unique_ptr<FactNames> clone() const override;

        [[nodiscard]] int get_num_operators() const override;

        [[nodiscard]] size_t get_num_variables() const override;
    };

    class FactValueNames {
    protected:
        std::shared_ptr<FactNames> fact_names;
        int variable;

    public:
        FactValueNames(const std::shared_ptr<FactNames>& fact_names, int variable);

        [[nodiscard]] std::string get_fact_value_name(const int value) const;

        [[nodiscard]] std::string get_operator_name(const int index, const bool is_axiom) const;

        [[nodiscard]] std::string get_common_operators_name(const std::vector<int>& ops) const;;
    };

    std::unique_ptr<FactValueNames> get_debug_or_release_fact_value_names(
        const std::shared_ptr<FactNames>& fact_names, int variable);

#ifndef NDEBUG
    // Only in debug, because this should never be used in release

    /**
     * This class is used to generate compact operator names. For a sequence of operators, it considers all the operator
     * names, and creates a compact representation of the common parts of the operator names, while factoring the entire
     * set of possible operator names, so as to only include information that is relevant in order to distinguish the
     * particular operators. This process is quite time-consuming, so it should in no way be used for non-debug related
     * purposes.
     */
    class NfaAggregatableFactValueNames : public FactValueNames {
    protected:
        boost::bimap<mata::Symbol, std::string> symbol_to_word;
        mata::nfa::Nfa full_nfa;
        mata::Alphabet* alphabet;
    public:
        NfaAggregatableFactValueNames(const std::shared_ptr<FactNames>& fact_names, int variable);


        [[nodiscard]] std::string get_fact_value_name(const int value) const;

        [[nodiscard]] std::string get_operator_name(const int index, const bool is_axiom) const;

        [[nodiscard]] std::string get_common_operators_name(const std::vector<int>& ops) const;

    private:
        std::vector<std::string> create_common_operator_names(const mata::nfa::Nfa& op_nfa, mata::nfa::State op_state, const mata::nfa::StateSet& full_states) const;
    };
#endif


} // fts

#endif //FACT_NAMES_H
