#include "eager_search.h"

#include "../evaluation_context.h"
#include "../evaluator.h"
#include "../open_list_factory.h"
#include "../pruning_method.h"

#include "../algorithms/ordered_set.h"
#include "../plugins/options.h"
#include "../task_utils/successor_generator.h"
#include "../utils/logging.h"

#include <cassert>
#include <cstdlib>
#include <memory>
#include <optional>
#include <set>

#include "../utils/search_space_draw.h"

using namespace std;

namespace eager_search {
EagerSearch::EagerSearch(
    const shared_ptr<OpenListFactory> &open, bool reopen_closed,
    const shared_ptr<Evaluator> &f_eval,
    const vector<shared_ptr<Evaluator>> &preferred,
    const shared_ptr<PruningMethod> &pruning,
    const shared_ptr<Evaluator> &lazy_evaluator, OperatorCost cost_type,
    int bound, double max_time, const string &description,
    utils::Verbosity verbosity)
    : SearchAlgorithm(
          cost_type, bound, max_time, description, verbosity),
      reopen_closed_nodes(reopen_closed),
      open_list(open->create_state_open_list()),
      f_evaluator(f_eval),     // default nullptr
      preferred_operator_evaluators(preferred),
      lazy_evaluator(lazy_evaluator),     // default nullptr
      pruning_method(pruning)
#ifndef NDEBUG
    ,search_space_drawer("search_space.dot")
#endif
    {
    if (lazy_evaluator && !lazy_evaluator->does_cache_estimates()) {
        cerr << "lazy_evaluator must cache its estimates" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
}

void EagerSearch::initialize() {
    log << "Conducting best first search"
        << (reopen_closed_nodes ? " with" : " without")
        << " reopening closed nodes, (real) bound = " << bound
        << endl;
    assert(open_list);

    set<Evaluator *> evals;
    open_list->get_path_dependent_evaluators(evals);

    /*
      Collect path-dependent evaluators that are used for preferred operators
      (in case they are not also used in the open list).
    */
    for (const shared_ptr<Evaluator> &evaluator : preferred_operator_evaluators) {
        evaluator->get_path_dependent_evaluators(evals);
    }

    /*
      Collect path-dependent evaluators that are used in the f_evaluator.
      They are usually also used in the open list and will hence already be
      included, but we want to be sure.
    */
    if (f_evaluator) {
        f_evaluator->get_path_dependent_evaluators(evals);
    }

    /*
      Collect path-dependent evaluators that are used in the lazy_evaluator
      (in case they are not already included).
    */
    if (lazy_evaluator) {
        lazy_evaluator->get_path_dependent_evaluators(evals);
    }

    path_dependent_evaluators.assign(evals.begin(), evals.end());

    State initial_state = state_registry.get_initial_state();
    for (Evaluator *evaluator : path_dependent_evaluators) {
        evaluator->notify_initial_state(initial_state);
    }
#ifndef NDEBUG
    search_space_drawer.set_initial_state(initial_state);
#endif

    /*
      Note: we consider the initial state as reached by a preferred
      operator.
    */
    EvaluationContext eval_context(initial_state, 0, true, &statistics);

    statistics.inc_evaluated_states();

    if (open_list->is_dead_end(eval_context)) {
        log << "Initial state is a dead end." << endl;
    } else {
        if (search_progress.check_progress(eval_context))
            statistics.print_checkpoint_line(0);
        start_f_value_statistics(eval_context);
        SearchNode node = search_space.get_node(initial_state);
        node.open_initial();

        open_list->insert(eval_context, initial_state.get_id());
    }

    print_initial_evaluator_values(eval_context);

    pruning_method->initialize(task);
}

void EagerSearch::print_statistics() const {
    statistics.print_detailed_statistics();
    search_space.print_statistics();
    pruning_method->print_statistics();
}

SearchStatus EagerSearch::step() {
    optional<SearchNode> node;
    while (true) {
        if (open_list->empty()) {
            log << "Completely explored state space -- no solution!" << endl;
#ifndef NDEBUG
            search_space_drawer.draw_search_space(task);
#endif
            return FAILED;
        }
        StateID id = open_list->remove_min();
        State s = state_registry.lookup_state(id);
        node.emplace(search_space.get_node(s));

        if (node->is_closed())
            continue;

        /*
          We can pass calculate_preferred=false here since preferred
          operators are computed when the state is expanded.
        */
        EvaluationContext eval_context(s, node->get_g(), false, &statistics);

        if (lazy_evaluator) {
            /*
              With lazy evaluators (and only with these) we can have dead nodes
              in the open list.

              For example, consider a state s that is reached twice before it is
              expanded. The first time we insert it into the open list, we
              compute a finite heuristic value. The second time we insert it,
              the cached value is reused.

              During first expansion, the heuristic value is recomputed and
              might become infinite, for example because the reevaluation uses a
              stronger heuristic or because the heuristic is path-dependent and
              we have accumulated more information in the meantime. Then upon
              second expansion we have a dead-end node which we must ignore.
            */
            if (node->is_dead_end())
                continue;

            if (lazy_evaluator->is_estimate_cached(s)) {
                int old_h = lazy_evaluator->get_cached_estimate(s);
                int new_h = eval_context.get_evaluator_value_or_infinity(lazy_evaluator.get());
                if (open_list->is_dead_end(eval_context)) {
                    node->mark_as_dead_end();
                    statistics.inc_dead_ends();
                    continue;
                }
                if (new_h != old_h) {
                    open_list->insert(eval_context, id);
                    continue;
                }
            }
        }

        node->close();
        assert(!node->is_dead_end());
        update_f_value_statistics(eval_context);
        statistics.inc_expanded();
        break;
    }

    const State &s = node->get_state();
    if (check_goal_and_set_plan(s)) {
#ifndef NDEBUG
        search_space_drawer.draw_search_space(task);
#endif
        return SOLVED;
    }

    //TODO: Right now the pruned nodes are actually closed. Perhaps we should set the state of the node to pruned?
    if(pruning_method->prune_state(s, node->get_info())) {
        return IN_PROGRESS;
    }

    vector<OperatorID> applicable_ops;
    successor_generator.generate_applicable_ops(s, applicable_ops);

    /*
      TODO: When preferred operators are in use, a preferred operator will be
      considered by the preferred operator queues even when it is pruned.
    */
    pruning_method->prune_operators(s, node->get_info(), applicable_ops);

    // This evaluates the expanded state (again) to get preferred ops
    EvaluationContext eval_context(s, node->get_g(), false, &statistics, true);
    ordered_set::OrderedSet<OperatorID> preferred_operators;
    for (const shared_ptr<Evaluator> &preferred_operator_evaluator : preferred_operator_evaluators) {
        collect_preferred_operators(eval_context,
                                    preferred_operator_evaluator.get(),
                                    preferred_operators);
    }

    for (OperatorID op_id : applicable_ops) {
        OperatorProxy op = task_proxy.get_operators()[op_id];
        if ((node->get_real_g() + op.get_cost()) >= bound)
            continue;

        State succ_state = state_registry.get_successor_state(s, op);
        statistics.inc_generated();
        bool is_preferred = preferred_operators.contains(op_id);

        SearchNode succ_node = search_space.get_node(succ_state);

        for (Evaluator *evaluator : path_dependent_evaluators) {
            evaluator->notify_state_transition(s, op_id, succ_state);
        }
#ifndef NDEBUG
        search_space_drawer.add_successor(s, op, succ_state);
#endif

        // Previously encountered dead end. Don't re-evaluate.
        if (succ_node.is_dead_end())
            continue;

        if (succ_node.is_new()) {
            /*
              We have not seen this state before.
              Evaluate and create a new node.

              Careful: succ_node.get_g() is not available here yet,
              hence the stupid computation of succ_g.
              TODO: Make this less fragile.
            */
            int succ_g = node->get_g() + get_adjusted_cost(op);

            EvaluationContext succ_eval_context(
                succ_state, succ_g, is_preferred, &statistics);
            statistics.inc_evaluated_states();
#ifndef NDEBUG
            if (succ_eval_context.is_evaluator_value_infinite(f_evaluator.get())) {
                search_space_drawer.set_heuristic_value(succ_state, EvaluationResult::INFTY);
            } else {
                search_space_drawer.set_heuristic_value(succ_state, succ_eval_context.get_evaluator_value(f_evaluator.get()) - succ_g);
            }
#endif

            if (open_list->is_dead_end(succ_eval_context)) {
                succ_node.mark_as_dead_end();
                statistics.inc_dead_ends();
                continue;
            }
            succ_node.open_new_node(*node, op, get_adjusted_cost(op));

            open_list->insert(succ_eval_context, succ_state.get_id());
            if (search_progress.check_progress(succ_eval_context)) {
                statistics.print_checkpoint_line(succ_node.get_g());
                reward_progress();
            }
        } else if (succ_node.get_g() > node->get_g() + get_adjusted_cost(op)) {
            // We found a new cheapest path to an open or closed state.
            if (succ_node.is_open()) {
                succ_node.update_open_node_parent(
                    *node, op, get_adjusted_cost(op));
                EvaluationContext succ_eval_context(
                succ_state, succ_node.get_g(), is_preferred, &statistics);
#ifndef NDEBUG
                if (succ_eval_context.is_evaluator_value_infinite(f_evaluator.get())) {
                    search_space_drawer.set_heuristic_value(succ_state, EvaluationResult::INFTY);
                } else {
                    search_space_drawer.set_heuristic_value(succ_state, succ_eval_context.get_evaluator_value(f_evaluator.get()) - succ_node.get_g());
                }
#endif
                open_list->insert(succ_eval_context, succ_state.get_id());
            } else if (succ_node.is_closed() && reopen_closed_nodes) {
                /*
                  TODO: It would be nice if we had a way to test
                  that reopening is expected behaviour, i.e., exit
                  with an error when this is something where
                  reopening should not occur (e.g. A* with a
                  consistent heuristic).
                */
                statistics.inc_reopened();
                succ_node.reopen_closed_node(*node, op, get_adjusted_cost(op));
                EvaluationContext succ_eval_context(succ_state, succ_node.get_g(), is_preferred, &statistics);
#ifndef NDEBUG
                if (succ_eval_context.is_evaluator_value_infinite(f_evaluator.get())) {
                    search_space_drawer.set_heuristic_value(succ_state, EvaluationResult::INFTY);
                } else {
                    search_space_drawer.set_heuristic_value(succ_state, succ_eval_context.get_evaluator_value(f_evaluator.get()) - succ_node.get_g());
                }
#endif
                open_list->insert(succ_eval_context, succ_state.get_id());
            } else {
                /*
                  If we do not reopen closed nodes, we just update the parent
                  pointers. Note that this could cause an incompatibility
                  between the g-value and the actual path that is traced back.
                */
                assert(succ_node.is_closed() && !reopen_closed_nodes);
                succ_node.update_closed_node_parent(
                    *node, op, get_adjusted_cost(op));
            }
        } else {
            /*
              We found an equally or more expensive path to an open or closed
              state.
            */
        }
    }
    return IN_PROGRESS;
}

void EagerSearch::reward_progress() {
    // Boost the "preferred operator" open lists somewhat whenever
    // one of the heuristics finds a state with a new best h value.
    open_list->boost_preferred();
}

void EagerSearch::dump_search_space() const {
    search_space.dump(task_proxy);
}

void EagerSearch::start_f_value_statistics(EvaluationContext &eval_context) {
    if (f_evaluator) {
        int f_value = eval_context.get_evaluator_value(f_evaluator.get());
        statistics.report_f_value_progress(f_value);
    }
}

/* TODO: HACK! This is very inefficient for simply looking up an h value.
   Also, if h values are not saved it would recompute h for each and every state. */
void EagerSearch::update_f_value_statistics(EvaluationContext &eval_context) {
    if (f_evaluator) {
        int f_value = eval_context.get_evaluator_value(f_evaluator.get());
        statistics.report_f_value_progress(f_value);
    }
}

void add_eager_search_options_to_feature(
    plugins::Feature &feature, const string &description) {
    add_search_pruning_options_to_feature(feature);
    // We do not add a lazy_evaluator options here
    // because it is only used for astar but not the other plugins.
    add_search_algorithm_options_to_feature(feature, description);
}

tuple<shared_ptr<PruningMethod>, shared_ptr<Evaluator>, OperatorCost,
      int, double, string, utils::Verbosity>
get_eager_search_arguments_from_options(const plugins::Options &opts) {
    return tuple_cat(
        get_search_pruning_arguments_from_options(opts),
        make_tuple(opts.get<shared_ptr<Evaluator>>(
                       "lazy_evaluator", nullptr)),
        get_search_algorithm_arguments_from_options(opts)
        );
}
}
