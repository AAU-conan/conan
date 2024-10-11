#include "qdom_lm_constraints.h"

#include "delete_relaxation_if_constraints.h"
#include "../qualified_dominance/qualified_dominance_pruning_local.h"
#include "../factored_transition_system/fts_task_factory.h"
#include "../factored_transition_system/draw_fts.h"
#include "../plugins/plugin.h"

#include <print>

namespace operator_counting {
    void QualifiedDominanceLandmarkConstraints::initialize_constraints(const std::shared_ptr<AbstractTask>& task, lp::LinearProgram& lp) {
        fts::AtomicTaskFactory fts_factory;
        const auto transformed_task = fts_factory.transform_to_fts(task);

#ifndef NDEBUG
        fts::draw_fts("fts.dot", *transformed_task.fts_task);
#endif

        factored_qdomrel = qdominance::QualifiedLDSimulation(utils::Verbosity::DEBUG).compute_dominance_relation(*transformed_task.fts_task);

        reachable.resize(factored_qdomrel->size());
        landmark_ops.resize(factored_qdomrel->size());
        state_to_stratum.resize(factored_qdomrel->size());
        stratification.resize(factored_qdomrel->size());
        for (int i = 0; i < factored_qdomrel->size(); ++i) {
            auto& factor_stratification = stratification.at(i);
            auto automaton = (*factored_qdomrel)[i].get_nfa();
            automaton.swap_final_nonfinal();

            // Maps each state s to the states that have a transition to s
            std::vector<std::set<mata::nfa::State>> state_premap = std::vector<std::set<mata::nfa::State>>(automaton.num_of_states());
            for (auto [from, label, to] : automaton.delta.transitions()) {
                state_premap[to].insert(from);
            }


            factor_stratification.emplace_back(automaton.final | std::ranges::to<std::vector<mata::nfa::State>>());
            auto& factor_state_to_stratum = state_to_stratum.at(i);
            factor_state_to_stratum.resize(automaton.num_of_states(), -1);
            for (auto q : automaton.final) {
                factor_state_to_stratum[q] = 0;
            }

            auto stratified = automaton.final;
            for (int stratum_i = 1;; ++stratum_i) {
                std::vector<mata::nfa::State> stratum;
                for (auto q : factor_stratification.at(stratum_i - 1)) {
                    for (auto pre : state_premap.at(q)) {
                        if (!stratified.contains(pre)) {
                            stratum.push_back(pre);
                            factor_state_to_stratum[pre] = stratum_i;
                            stratified.insert(pre);
                        }
                    }
                }
                if (stratum.empty()) {
                    break;
                }
                factor_stratification.emplace_back(stratum);
            }

            // Maps each state s to the states that s can reach
            auto& factor_reachability = reachable.at(i);
            for (auto [from, label, to] : automaton.delta.transitions()) {
                factor_reachability[from].insert(to);
            }
            // Compute closure
            bool changed = false;
            do {
                changed = false;
                for (int q = 0; q < automaton.num_of_states(); ++q) {
                    factor_reachability[q].insert(q);
                    auto size_before = factor_reachability[q].size();
                    for (auto r : factor_reachability[q]) {
                        factor_reachability[q].insert(factor_reachability[r].begin(), factor_reachability[r].end());
                    }
                    changed |= factor_reachability[q].size() != size_before;
                }
            } while (changed);

            auto& factor_landmark_ops = landmark_ops.at(i);
            factor_landmark_ops.resize(automaton.num_of_states());
            for (auto [from, label, to] : automaton.delta.transitions()) {
                auto from_statum = factor_state_to_stratum[from];
                auto to_stratum = factor_state_to_stratum[to];
                if (from_statum != -1 && to_stratum != -1 && from_statum - 1 == to_stratum) {
                    factor_landmark_ops[from].push_back(label);
                }
            }
        }
    }

    bool QualifiedDominanceLandmarkConstraints::update_constraints_g_value(const State& state, int g_value, lp::LPSolver& lp_solver) {
        state.unpack();
        auto explicit_state = state.get_unpacked_values();


        LPConstraints lp_constraints;
        for (const auto& previous_state : previous_states) {
            if (previous_state.g_value <= g_value) {
                for (int stratum = 0;; ++stratum) {
                    // Constraint that for each factor, if the stratum of the state is greater than or equal to 'stratum', then we must apply one of the operators
                    lp::LPConstraint stratum_landmark(1., lp_solver.get_infinity());
                    for (int i = 0; i < previous_state.state.size(); ++i) {
                        mata::nfa::State q = (*factored_qdomrel)[i].nfa_simulates(explicit_state[i], previous_state.state[i]);
                        if (state_to_stratum.at(i).at(q) >= stratum) {
                            for (auto p : stratification.at(i).at(stratum)) {
                                if (reachable.at(i).at(q).contains(p)) {
                                    for (auto op : landmark_ops.at(i).at(p)) {
                                        stratum_landmark.insert(op, 1.);
                                    }
                                }
                            }
                        }
                    }
                    if (stratum_landmark.get_variables().empty()) {
                        break;
                    }
                    lp_constraints.push_back(stratum_landmark);
                }
            }
        }

        previous_states.emplace_back(explicit_state, g_value);
        return false;
    }


    class QualifiedDominanceLandmarkConstraintsFeature
    : public plugins::TypedFeature<ConstraintGenerator, QualifiedDominanceLandmarkConstraints> {
    public:
        QualifiedDominanceLandmarkConstraintsFeature() : TypedFeature("qdom_lm_constraints") {
            document_title("Qualified Dominance Landmark constraints");
            document_synopsis("");
        }

        [[nodiscard]] std::shared_ptr<QualifiedDominanceLandmarkConstraints> create_component(const plugins::Options &, const utils::Context &) const override {
            return std::make_shared<QualifiedDominanceLandmarkConstraints>();
        }
    };

    static plugins::FeaturePlugin<QualifiedDominanceLandmarkConstraintsFeature> _plugin;
}
