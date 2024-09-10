#include "fts_task.h"

#include <cassert>
#include <format>

#include "../utils/system.h"
#include "labelled_transition_system.h"
#include "../merge_and_shrink/factored_transition_system.h"
#include "../merge_and_shrink/labels.h"
#include "label_map.h"

namespace fts {

    FTSTask::FTSTask(const merge_and_shrink::FactoredTransitionSystem &fts) {

        LabelMap label_map (fts.get_labels());
        NoFactNames fact_names;
        for (const auto & ts : fts) {
            transition_systems.push_back(std::make_unique<LabelledTransitionSystem>(fts.get_transition_system(ts), label_map, FactValueNames(fact_names, ts)));
        }

        label_costs.resize(label_map.get_num_labels());

        for (size_t i = 0; i < label_costs.size(); ++i) {
            label_costs[i] = fts.get_labels().get_label_cost(label_map.get_old_id(i));
        }
    }

    FTSTask::FTSTask(const merge_and_shrink::FactoredTransitionSystem &fts, const std::shared_ptr<AbstractTask>& parent)
    : fact_names(std::make_unique<AbstractTaskFactNames>(parent)) {

        LabelMap label_map (fts.get_labels());
        for (const auto & ts : fts) {
            transition_systems.push_back(std::make_unique<LabelledTransitionSystem>(fts.get_transition_system(ts), label_map, FactValueNames(*fact_names, ts)));
        }

        label_costs.resize(label_map.get_num_labels());
        for (size_t i = 0; i < label_costs.size(); ++i) {
            label_costs[i] = fts.get_labels().get_label_cost(label_map.get_old_id(i));
        }
    }

    int FTSTask::get_num_labels() const {
        return label_costs.size();
    }

    int FTSTask::get_label_cost(int label) const {
        assert ((size_t)label < label_costs.size());
        return label_costs[label];
    }

    const LabelledTransitionSystem &FTSTask::get_factor(int factor) const {
        assert ((size_t)factor < transition_systems.size());
        return *(transition_systems[factor]);
    }

    int FTSTask::get_num_variables() const {
        return transition_systems.size();
    }

    std::string FTSTask::get_variable_name(int var) const {
        return fact_names->get_variable_name(var);
    }

    int FTSTask::get_variable_domain_size(int var) const {
        return transition_systems[var]->size();
    }

    int FTSTask::get_variable_axiom_layer(int ) const {
        return -1; // We do not consider derived variables on FTSTasks
    }

    int FTSTask::get_variable_default_axiom_value(int /*var*/) const {
        ABORT("Accessing axioms of an FTSTask");    }

    std::string FTSTask::get_fact_name(const FactPair &fact) const {
        return fact_names->get_fact_name(fact);
    }

    bool FTSTask::are_facts_mutex(const FactPair &, const FactPair &) const {
        return false; // TODO: Mutex information should not be part of the task
    }

    int FTSTask::get_operator_cost(int index, bool is_axiom) const {
        assert(!is_axiom);
        return get_label_cost(index);
    }

    std::string FTSTask::get_operator_name(int index, bool is_axiom) const {
        assert (!is_axiom);
        return fact_names->get_operator_name(index, is_axiom);
    }

    int FTSTask::get_num_operators() const {
        return label_costs.size();
    }

    int FTSTask::get_num_operator_preconditions(int , bool ) const {
        return 0;
    }

    FactPair FTSTask::get_operator_precondition(int , int , bool ) const {
        ABORT("Accessing operator precondition of an FTSTask");
    }

    int FTSTask::get_num_operator_effects(int , bool ) const {
        return 0;
    }

    int FTSTask::get_num_operator_effect_conditions(int , int , bool ) const {
        ABORT("Accessing operator condition of an FTSTask");
    }

    FactPair FTSTask::get_operator_effect_condition(int , int , int , bool ) const {
        ABORT("Accessing operator condition of an FTSTask");
    }

    FactPair FTSTask::get_operator_effect(int , int , bool ) const {
        ABORT("Accessing operator effects of an FTSTask");
    }

    int FTSTask::get_num_axioms() const {
        return 0;
    }

    int FTSTask::get_num_goals() const {
        ABORT("Accessing num_goals of an FTSTask");
    }

    FactPair FTSTask::get_goal_fact(int ) const {
        ABORT("Accessing goal of an FTSTask");
    }

    std::vector<int> FTSTask::get_initial_state_values() const {
        std::vector<int> initial_state_values;
        for (const auto & factor : transition_systems) {
            initial_state_values.push_back(factor->get_initial_state());
        }
        return initial_state_values;
    }

    void FTSTask::convert_ancestor_state_values(std::vector<int> &, const AbstractTask *ancestor_task) const {
        // TODO: One could actually store the state mapping as part of the FTSTask.
        // However, I prefer to keep it separate so that the tasks do not necessarily know where they come from
        // That's a design decision that should be considered to fully integrate FTSTasks
        if (this != ancestor_task) {
            ABORT("Invalid state conversion");
        }
    }

    int FTSTask::convert_operator_index(int , const AbstractTask *ancestor_task) const {
        if (this != ancestor_task) {
            ABORT("Invalid operator conversion");
        }
        return 0;
    }

    const std::vector<std::unique_ptr<LabelledTransitionSystem>> &FTSTask::get_factors() const {
        return transition_systems;
    }

}