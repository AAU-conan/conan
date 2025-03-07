#include "uniform_cost_search.h"

#include "sym_controller.h"
#include "closed_list.h"
#include "sym_solution.h"
#include "transition_relation.h"

#include <string>
#include <utility>

#include "opposite_frontier.h"


using namespace std;
using utils::g_timer;
using utils::Timer;

namespace symbolic {
    UniformCostSearch::UniformCostSearch(const SymParamsSearch &params,
                                         const std::shared_ptr<SymStateSpaceManager> &manager,
                                         bool fw, std::shared_ptr<SymSolutionLowerBound> solution) : UniformCostSearch(params, manager, fw,
                                                                      fw
                                                                          ? manager->getInitialState()
                                                                          : manager->getGoal(),
                                                                          make_shared<ClosedList>(manager),
                                                                          (params.non_stop ?
                                                                          static_cast<shared_ptr<OppositeFrontier>> (make_shared<OppositeFrontierNonStop>(manager)) :
                                                                          static_cast<shared_ptr<OppositeFrontier>> (make_shared<OppositeFrontierFixed>(manager, fw))),
                                                                          solution) {
    }

    UniformCostSearch::UniformCostSearch(const SymParamsSearch &params,
                                         const std::shared_ptr<SymStateSpaceManager> &manager, bool fw,
                                         const BDD &init_bdd, shared_ptr<ClosedList> closed,
                                         shared_ptr<OppositeFrontier> frontier,
                                         std::shared_ptr<SymSolutionLowerBound> solution) : SymSearch(params, manager,
            solution), fw(fw), open_list(*manager), closed(closed),
        perfectHeuristic(frontier), estimation(params) {
        if (mgr->hasTransitions0()) {
            open_list.insert_zero(0, init_bdd);
        } else {
            open_list.insert_cost(0, init_bdd);
        }
        closed->init(init_bdd, open_list.get_min_new_g());

        //TODO: Check against goal to detect cases where the initial state is a goal state

        solution->setLowerBound(getF(), p.log);
    }

    /*
                frontier.filter(!closed->notClosed());
                if (perfectHeuristic && perfectHeuristic->exhausted()) {
                    frontier.filter(perfectHeuristic->notClosed());
                }
                mgr->filterDead(frontier.bucket(), fw, initialization());
                removeZero(frontier.bucket());
    */


    bool UniformCostSearch::stepImage(utils::Duration maxTime, long maxNodes) {
        Timer sTime;
        StepImage step_image = open_list.pop();

        if (p.log.is_at_least_debug()) {
            p.log << ">> Step: " << *mgr << (fw ? " fw" : " bw") << ", " << step_image << " num states: " << mgr->
                    getVars()->numStates(step_image.bdd) << " total nodes: " << mgr->totalNodes() << endl;
        } else if (p.log.is_at_least_verbose()) {
            p.log << ">> Step: " << *mgr << (fw ? " fw" : " bw") << ", " << step_image << endl;
        }

        auto maybe_generated_states = mgr->getVars()->get_bdd_manager()->compute_bdd_with_time_limit(maxTime, [&]() {
            return step_image.transition_relation->image(fw, step_image.bdd, maxNodes);
        });

        if (!maybe_generated_states.has_value()) {
            p.log << "Step failed" << endl;
            //TODO: estimation.violated()
            open_list.insert(step_image);
            stats.add_image_time_failed(sTime());
            return false;
        }

        BDD generated_states = maybe_generated_states.value();
        auto time_image = sTime();
        stats.add_image_time(time_image);
        estimation.stepTaken(time_image, step_image.bdd.nodeCount());
        if (p.log.is_at_least_debug()) {
            p.log << "Generated states: " << generated_states.nodeCount() << " num states: " << mgr->getVars()->
                    numStates(generated_states) << " time image: " << time_image << endl;
        }

        if (!generated_states.IsZero()) {
            //Check the cut (removing states classified, since they do not need to be included in open)
            auto sol = perfectHeuristic->checkCut(closed, generated_states, step_image.new_g(), fw);
            if (sol) {
                // Solution found :)
                solution->new_solution(*sol, p.log);
            }

            //TODO: We should reconsider whether the commented out operation makes sense. I don't think so because
            //      if we already found a solution we should not be expanding those states,
            //      and duplicate detection should be performed somewhere else.
            //generated_states *= perfectHeuristic->notClosed();   //Prune everything closed in opposite direction

            closed->put_in_frontier(step_image.new_g(), generated_states);

            if (solution->solved()) {
                //TODO: Here we are not closing the states, which may be a problem for heuristic generation from the closed list
                //            DEBUG_MSG(p.log << "SOLVED!!!: " << engine->getLowerBound() << " >= " << engine->getUpperBound() << endl;);
                return true; //If it has been solved, return
            }
        }

        closed->closeUpTo(open_list, maxTime, maxNodes);

        solution->setLowerBound(getF(), p.log);

        stats.step_time += sTime();
        if (!finished()) {
            estimation.nextStep(open_list.nextStep().bdd.nodeCount());
        }

        if (p.log.is_at_least_debug()) {
            p.log << "Step finished" << endl;
        }
        return true;
    }

    bool UniformCostSearch::isSearchableWithNodes(int maxNodes) const {
        return nextStepNodes() <= maxNodes;
    }

    utils::Duration UniformCostSearch::nextStepTime() const {
        return estimation.time();
    }

    long UniformCostSearch::nextStepNodes() const {
        return estimation.nodes();
    }

    std::ostream &operator<<(std::ostream &os, const UniformCostSearch &exp) {
        os << "exp " << dirname(exp.isFW());
        if (exp.mgr) {
            os << " in " << *(exp.mgr)
                    << " g=" << exp.getG() << flush
                    << exp.open_list << flush
                    << " est_time: " << exp.nextStepTime() << flush
                    << " est_nodes: " << exp.nextStepNodes() << flush;
        }
        return os;
    }

    void UniformCostSearch::getPlan(const BDD &cut, int g, std::vector<OperatorID> &path) const {
        closed->extract_path(cut, g, fw, path);
        if (fw) {
            ranges::reverse(path);
        }
    }

    bool UniformCostSearch::finished() const {
//        assert(!open_list.empty() || closed->getGNotClosed() == std::numeric_limits<int>::max());
        return open_list.empty();
    }

    OpenList::OpenList(const SymStateSpaceManager &mgr) {
        for (const auto &[cost, trs]: mgr.getTransitions()) {
            for (const auto &tr: trs) {
                if (tr->getCost()) {
                    transition_relations_cost.push_back(tr);
                } else {
                    transition_relations_zero.push_back(tr);
                }
            }
        }
    }

    void OpenList::insert_zero(int state_cost, BDD bdd) {
        for (const auto &tr: transition_relations_zero) {
            open[state_cost + tr->getCost()].emplace_back(state_cost, bdd, tr.get());
        }
    }

    void OpenList::insert_cost(int state_cost, BDD bdd) {
        for (const auto &tr: transition_relations_cost) {
            open[state_cost + tr->getCost()].emplace_back(state_cost, bdd, tr.get());
        }
    }

    void OpenList::insert(const StepImage &task) {
        open[task.new_g()].push_back(task);
    }

    int OpenList::get_min_new_g() const {
        return open.empty() ? std::numeric_limits<int>::max() : open.begin()->first;
    }

    int OpenList::get_min_old_g() const {
        int value =std::numeric_limits<int>::max();
        // TODO: We could keep track of the minimum value in a different way. Unclear if this is needed in the first place,
        // or how to organize open in a better way
        for (const auto & steps : open ) {
            for (const auto & step : steps.second) {
                value = std::min(value, step.old_g());
            }
        }
        return value;
    }


    bool OpenList::empty() const {
        return open.empty();
    }

    StepImage OpenList::pop() {
        assert(!open.empty());
        assert(!open.begin()->second.empty());
        auto &tasks = open.begin()->second;
        StepImage next = tasks.back();
        tasks.pop_back();
        if (tasks.empty()) {
            open.erase(open.begin());
        }
        return next;
    }

    const StepImage & OpenList::nextStep() const {
        assert(!open.empty());
        assert(!open.begin()->second.empty());
        auto &tasks = open.begin()->second;
        return tasks.back();
    }

    void UniformCostSearch::statistics() const {
        p.log << "Exp " << (fw ? "fw" : "bw") << " time: " << stats.step_time << "s (img:" << stats.image_time
                << "s, heur: " << stats.time_heuristic_evaluation << "s) in " << stats.num_steps_succeeded << " steps ";
    }

    std::ostream &operator<<(std::ostream &os, const OpenList &exp) {
        os << " open{";
        for (auto &o: exp.open) {
            os << o.first << " ";
        }
        return os << "}";
    }

    std::ostream &operator<<(std::ostream &os, const StepImage &exp) {
        os << "g=" << exp.new_g() << " from " << exp.old_g() << " (" << exp.bdd.nodeCount() << " nodes) with "
                << *(exp.transition_relation);
        return os;
    }

    int StepImage::new_g() const {
        return state_cost + transition_relation->getCost();
    }

    StepImage::StepImage(int stateCost, const BDD &bdd, TransitionRelation *transitionRelation) : state_cost(stateCost),
        bdd(bdd),
        transition_relation(
            transitionRelation) {
    }
}
