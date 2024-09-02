#include "uniform_cost_search.h"

#include "sym_controller.h"
#include "closed_list.h"
#include "sym_solution.h"
#include "merge_bdds.h"
#include "transition_relation.h"

#include <iostream>
#include <string>
#include <utility>


using namespace std;
using utils::g_timer;
using utils::Timer;

namespace symbolic {
    UniformCostSearch::UniformCostSearch(SymController *eng, const SymParamsSearch &params) :
            UnidirectionalSearch(eng, params), estimation(params) {}

    void UniformCostSearch::init(const std::shared_ptr<SymStateSpaceManager>& manager, bool forward) {
        assert(manager);

        if (forward) {
            init(manager, forward, manager->getInitialState(), make_shared<OppositeFrontierFixed>(manager->getGoal(), *manager));
        } else {
            init(manager, forward, manager->getGoal(), make_shared<OppositeFrontierFixed>(manager->getInitialState(), *manager));
        }
    }

    void UniformCostSearch::init(const std::shared_ptr<SymStateSpaceManager>& manager, bool forward, const BDD& init_bdd, shared_ptr<OppositeFrontier> frontier){
        mgr = manager;
        fw = forward;
        assert(mgr);

        perfectHeuristic = std::move(frontier);

        open_list.init(*manager);
        if (mgr->hasTransitions0()) {
            open_list.insert_zero(0, init_bdd);
        } else {
            open_list.insert_cost(0, init_bdd);
        }
        closed->init(mgr.get(), this, init_bdd, open_list.minG());

        engine->setLowerBound(getG());
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
            p.log << ">> Step: " << *mgr << (fw ? " fw" : " bw") << ", " << step_image <<  " num states: " << mgr->getVars()->numStates(step_image.bdd) << " total nodes: " << mgr->totalNodes() << endl;
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
            p.log << "Generated states: " << generated_states.nodeCount() <<  " num states: " << mgr->getVars()->numStates(generated_states) << " time image: " << time_image << endl;
        }

        if (!generated_states.IsZero()) {
            //Check the cut (removing states classified, since they do not need to be included in open)
            //TODO: if (!isAbstracted()) This is kind of old code, that requires searches in abstractions to not check against the other frontier.
            // But that only makes sense on SymBA's heuristic and could be implemented more elegantly anyway.
            SymSolution sol = perfectHeuristic->checkCut(this, generated_states, step_image.g(), fw);
            if (sol.solved()) {
                p.log << "Solution found with cost " << sol.getCost() << " total time: " << g_timer << endl;
                // Solution found :)
                engine->new_solution(sol);
            }

            //TODO: We should reconsider whether the commented out operation makes sense. I don't think so because
            //      if we already found a solution we should not be expanding those states,
            //      and duplicate detection should be performed somewhere else.
            //generated_states *= perfectHeuristic->notClosed();   //Prune everything closed in opposite direction

            if (engine->solved()) {
                //            DEBUG_MSG(p.log << "SOLVED!!!: " << engine->getLowerBound() << " >= " << engine->getUpperBound() << endl;);
                return true; //If it has been solved, return
            }
            closed->put_in_frontier(step_image.g(), generated_states);
        }

        closed->closeUpTo(open_list, maxTime, maxNodes);

        engine->setLowerBound(getG());

        stats.step_time += sTime();

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

    ADD UniformCostSearch::getHeuristic() const {
        return closed->getHeuristic();
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
            std::reverse(path.begin(), path.end());
        }
    }

    void OpenList::init(const SymStateSpaceManager &mgr) {
        open.clear();
        transition_relations_cost.clear();
        transition_relations_zero.clear();
        for (const auto &[cost, trs]: mgr.getTransitions()) {
            for (const auto &tr: trs) {
                if(tr->getCost()) {
                    transition_relations_cost.push_back(tr);
                } else {
                    transition_relations_zero.push_back(tr);
                }
            }
        }
    }

    void OpenList::insert_zero(int state_cost, BDD bdd) {
        for (const auto & tr: transition_relations_zero) {
            open[state_cost + tr->getCost()].emplace_back(state_cost, bdd, tr.get());
        }
    }

    void OpenList::insert_cost(int state_cost, BDD bdd) {
        for (const auto & tr: transition_relations_cost) {
            open[state_cost + tr->getCost()].emplace_back(state_cost, bdd, tr.get());
        }
    }


    void OpenList::insert(StepImage task) {
        open[task.g()].push_back(task);
    }

    int OpenList::minG() const {
        return open.empty() ? std::numeric_limits<int>::max() :
               open.begin()->first;
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

    std::ostream &operator<<(std::ostream &os, const OpenList &exp) {
        os << " open{";
        for (auto &o: exp.open) {
            os << o.first << " ";
        }
        return os << "}";
    }

    std::ostream &operator<<(std::ostream &os, const StepImage &exp) {
        os << "g=" << exp.g() << " from " << exp.state_cost << " (" << exp.bdd.nodeCount() << " nodes) with "
           << *(exp.transition_relation);
        return os;
    }

    int StepImage::g() const {
        return state_cost + transition_relation->getCost();
    }

    StepImage::StepImage(int stateCost, const BDD &bdd, TransitionRelation *transitionRelation) : state_cost(stateCost),
                                                                                                  bdd(bdd),
                                                                                                  transition_relation(
                                                                                                          transitionRelation) {}

}
