#include "original_state_space.h"

#include "transition_relation.h"
#include "../task_proxy.h"
#include <algorithm>

using namespace std;

namespace symbolic {
    OriginalStateSpace::OriginalStateSpace(SymVariables *v,
                                           const SymParamsMgr &params,
                                           const shared_ptr <AbstractTask> &_task) :
            SymStateSpaceManager(v, params), task(_task) {

        TaskProxy t(*task);

        initialState = vars->getStateBDD(t.get_initial_state());

        std::vector<FactPair> goals;
        for (const auto &g: t.get_goals()) {
            goals.push_back(g.get_pair());
        }
        goal = vars->getPartialStateBDD(goals);

        for (const auto &op: t.get_operators()) {
            indTRs[op.get_cost()].push_back(make_shared<TransitionRelation>(vars, op, op.get_cost()));
        }

        init_transitions(indTRs);
    }


}





