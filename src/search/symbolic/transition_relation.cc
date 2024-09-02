#include "transition_relation.h"

#include <algorithm>
#include <cassert>
#include <ranges>

#include "../task_proxy.h"

using namespace std;

namespace symbolic {
    TransitionRelation::TransitionRelation(SymVariables *sVars, const OperatorProxy &op, int cost_) :
            sV(sVars), cost(cost_), tBDD(sVars->oneBDD()),
            existsVars(sVars->oneBDD()), existsBwVars(sVars->oneBDD()) {
        ops.insert(OperatorID(op.get_id()));

        for (const auto &pre: op.get_preconditions()) {
            tBDD *= sV->preBDD(pre.get_variable().get_id(), pre.get_value());
        }

        map<int, BDD> effect_conditions;
        map<int, BDD> effects;
        // Get effects and the remaining conditions. We iterate in reverse
        // order because pre_post at the end have preference
        for (int i = op.get_effects().size() - 1; i >= 0; i--) {
            const auto &effect = op.get_effects()[i];
            int var = effect.get_fact().get_variable().get_id();
            if (std::find(effVars.begin(), effVars.end(), var) == effVars.end()) {
                effVars.push_back(var);
            }

            BDD condition = sV->oneBDD();
            BDD ppBDD = sV->effBDD(var, effect.get_fact().get_value());
            if (effect_conditions.count(var)) {
                condition = effect_conditions.at(var);
            } else {
                effect_conditions[var] = condition;
                effects[var] = sV->zeroBDD();
            }

            for (const auto &cPrev: effect.get_conditions()) {
                condition *= sV->preBDD(cPrev.get_variable().get_id(), cPrev.get_value());
            }
            effect_conditions[var] *= !condition;
            effects[var] += (condition * ppBDD);
        }

        //Add effects to the tBDD
        for (auto &[var, effectBDD]: std::ranges::reverse_view(effects)) {
            //If some possibility is not covered by the conditions of the
            //conditional effect, then in those cases the value of the value
            //is preserved with a biimplication
            if (!effect_conditions[var].IsZero()) {
                effectBDD += (effect_conditions[var] * sV->biimp(var));
            }
            tBDD *= effectBDD;
        }
        if (tBDD.IsZero()) {
            cerr << "Warning: an operator has been disambiguated away when constructing the transition relation: "
                 << op.get_name() << endl;
        }

        sort(effVars.begin(), effVars.end());
        for (int var: effVars) {
            for (int bdd_var: sV->vars_index_pre(var)) {
                swapVarsS.push_back(sV->bddVar(bdd_var));
            }
            for (int bdd_var: sV->vars_index_eff(var)) {
                swapVarsSp.push_back(sV->bddVar(bdd_var));
            }
        }
        assert(swapVarsS.size() == swapVarsSp.size());
        // existsVars/existsBwVars is just the conjunction of swapVarsS and swapVarsSp
        for (size_t i = 0; i < swapVarsS.size(); ++i) {
            existsVars *= swapVarsS[i];
            existsBwVars *= swapVarsSp[i];
        }
//    DEBUG_MSG(cout << "Computing TR took " << tr_timer; tBDD.print(1, 1););
    }

    BDD TransitionRelation::image(const BDD &from) const {
        BDD aux = from;
        if (!swapVarsA.empty()) {
            aux = from.SwapVariables(swapVarsA, swapVarsAp);
        }
        BDD tmp = tBDD.AndAbstract(aux, existsVars);
        BDD res = tmp.SwapVariables(swapVarsS, swapVarsSp);

        return res;
    }

    BDD TransitionRelation::image(const BDD &from, long maxNodes) const {
        //DEBUG_MSG(cout << "Image cost " << cost << " from " << from.nodeCount() << " with " << tBDD.nodeCount(););
        BDD aux = from;
        if (!swapVarsA.empty()) {
            aux = from.SwapVariables(swapVarsA, swapVarsAp);
        }
        //utils::Timer t;
        BDD tmp = tBDD.AndAbstract(aux, existsVars, maxNodes);
        //DEBUG_MSG(cout << " tmp " << tmp.nodeCount() << " in " << t(););
        BDD res = tmp.SwapVariables(swapVarsS, swapVarsSp);
        //DEBUG_MSG(cout << " res " << res.nodeCount() << " took " << t() << endl;);

        return res;
    }

    BDD TransitionRelation::preimage(const BDD &from) const {
        BDD tmp = from.SwapVariables(swapVarsS, swapVarsSp);
        BDD res = tBDD.AndAbstract(tmp, existsBwVars);
        if (!swapVarsA.empty()) {
            res = res.SwapVariables(swapVarsA, swapVarsAp);
        }
        return res;
    }

    BDD TransitionRelation::preimage(const BDD &from, long maxNodes) const {
        // utils::Timer t;
        //DEBUG_MSG(cout << "Image cost " << cost << " from " << from.nodeCount() << " with " << tBDD.nodeCount() << flush;);
        BDD tmp = from.SwapVariables(swapVarsS, swapVarsSp);
        //DEBUG_MSG(cout << " tmp " << tmp.nodeCount() << " in " << t() << flush;);
        BDD res = tBDD.AndAbstract(tmp, existsBwVars, maxNodes);
        if (!swapVarsA.empty()) {
            res = res.SwapVariables(swapVarsA, swapVarsAp);
        }
//    DEBUG_MSG(cout << "res " << res.nodeCount() << " took " << t() << endl; );

        return res;
    }

    void TransitionRelation::merge(const TransitionRelation &t2, long maxNodes) {
        assert(cost == t2.cost);
        if (cost != t2.cost) {
            cout << "Error: merging transitions with different cost: " << cost << " " << t2.cost << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }

        //  cout << "set_union" << endl;
        //Attempt to generate the new tBDD
        vector<int> newEffVars;
        set_union(effVars.begin(), effVars.end(),
                  t2.effVars.begin(), t2.effVars.end(),
                  back_inserter(newEffVars));

        BDD newTBDD = tBDD;
        BDD newTBDD2 = t2.tBDD;

        auto var1 = effVars.begin();
        auto var2 = t2.effVars.begin();
        for (const auto &var: newEffVars) {
            if (var1 == effVars.end() || *var1 != var) {
                newTBDD *= sV->biimp(var);
            } else {
                ++var1;
            }

            if (var2 == t2.effVars.end() || *var2 != var) {
                newTBDD2 *= sV->biimp(var);
            } else {
                ++var2;
            }
        }
        newTBDD = newTBDD.Or(newTBDD2, maxNodes);

        if (newTBDD.nodeCount() > maxNodes) {
            /* DEBUG_MSG(cout << "TR size exceeded: " << newTBDD.nodeCount() << ">" << maxNodes << endl;);*/
            throw BDDError(); //We could not successfully merge
        }

        tBDD = newTBDD;

        effVars.swap(newEffVars);
        existsVars *= t2.existsVars;
        existsBwVars *= t2.existsBwVars;

        for (size_t i = 0; i < t2.swapVarsS.size(); i++) {
            if (find(swapVarsS.begin(), swapVarsS.end(), t2.swapVarsS[i]) ==
                swapVarsS.end()) {
                swapVarsS.push_back(t2.swapVarsS[i]);
                swapVarsSp.push_back(t2.swapVarsSp[i]);
            }
        }

        ops.insert(t2.ops.begin(), t2.ops.end());
    }

    ostream &operator<<(std::ostream &os, const TransitionRelation &tr) {
/*
    for (auto op : tr.ops) {
        os << op << ", ";
    }
*/
        return os << "TR(" << tr.ops.size() << " ops, " << tr.tBDD.nodeCount() << " nodes)";
    }


    std::shared_ptr<TransitionRelation>
    merge_uniqueTR(const std::shared_ptr<TransitionRelation>& tr, const std::shared_ptr<TransitionRelation> &tr2,
                   int maxSize);

    std::shared_ptr<TransitionRelation>
    merge_uniqueTR(const shared_ptr <TransitionRelation> &tr, const shared_ptr <TransitionRelation> &tr2, int maxSize) {
        std::shared_ptr<TransitionRelation> merged = std::make_shared<TransitionRelation>(*tr);
        merged->merge(*tr2, maxSize);
        return merged;
    }

}
