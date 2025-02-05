#ifndef FTS_FTS_TASK_H
#define FTS_FTS_TASK_H

#include <vector>
#include <memory>

#include "../abstract_task.h"
#include "labelled_transition_system.h"

namespace merge_and_shrink {
    class FactoredTransitionSystem;
}

namespace fts {
    // This is very similar to the merge_and_shrink::FactoredTransitionSystem
    // However, the idea is that here, we have a "clean" unmutable version of the representation.
    // Specifically, we do not have "non-active" labels, or transition_systems, so all IDs are set from 0 to n-1.
    // Also, we do not have dead labels, unreachable or dead end states
    //
    // Also, we use a representation of transition systems with some redundant access to the transitions.
    // This adds some memory overhead as well as some overhead when copying the transition systems, but
    // it allows for faster access to the transitions from/to certain states.
    // Preliminary experiments showed that this can pay off for the computation of dominance relations

    class FTSTask : public AbstractTask {
        // The abstract task that was used to generate this task. This is optional.
        // For now just used to preserve fact and action names, whenever they match
        std::shared_ptr<FactNames> fact_names;

        std::vector<int> label_costs;
        std::vector<std::unique_ptr<LabelledTransitionSystem>> transition_systems;


    public:
        FTSTask(const merge_and_shrink::FactoredTransitionSystem & fts);
        FTSTask(const merge_and_shrink::FactoredTransitionSystem & fts, const std::shared_ptr<AbstractTask>& parent);


        int get_num_labels() const;

        int get_label_cost(int label) const;

        const LabelledTransitionSystem &get_factor(int i) const;

        int get_num_variables() const override;

        std::string get_variable_name(int var) const override;

        int get_variable_domain_size(int var) const override;

        int get_variable_axiom_layer(int var) const override;

        int get_variable_default_axiom_value(int var) const override;

        std::string get_fact_name(const FactPair &fact) const override;

        bool are_facts_mutex(const FactPair &fact1, const FactPair &fact2) const override;

        int get_operator_cost(int index, bool is_axiom) const override;

        std::string get_operator_name(int index, bool is_axiom) const override;

        int get_num_operators() const override;

        int get_num_operator_preconditions(int index, bool is_axiom) const override;

        FactPair get_operator_precondition(int op_index, int fact_index, bool is_axiom) const override;

        int get_num_operator_effects(int op_index, bool is_axiom) const override;

        int get_num_operator_effect_conditions(int op_index, int eff_index, bool is_axiom) const override;

        FactPair
        get_operator_effect_condition(int op_index, int eff_index, int cond_index, bool is_axiom) const override;

        FactPair get_operator_effect(int op_index, int eff_index, bool is_axiom) const override;

        int convert_operator_index(int index, const AbstractTask *ancestor_task) const override;

        int get_num_axioms() const override;

        int get_num_goals() const override;

        FactPair get_goal_fact(int index) const override;

        std::vector<int> get_initial_state_values() const override;

        void convert_ancestor_state_values(std::vector<int> &values, const AbstractTask *ancestor_task) const override;

        const std::vector<std::unique_ptr<LabelledTransitionSystem>> &get_factors() const;
    };

}

#endif
