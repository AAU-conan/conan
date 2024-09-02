#ifndef SYMBOLIC_TRANSITION_RELATION_H
#define SYMBOLIC_TRANSITION_RELATION_H

#include "sym_variables.h"
#include "../operator_id.h"

#include <set>
#include <vector>

class OperatorProxy;

namespace symbolic {
    class SymStateSpaceManager;

/*
 * Represents a symbolic transition.
 * It has two differentiated parts: label and abstract state transitions
 * Label refers to variables not considered in the merge-and-shrink
 * Each label has one or more abstract state transitions
 */
    class TransitionRelation {
        SymVariables *sV; //To call basic BDD creation methods
        int cost; // transition cost
        BDD tBDD; // bdd for making the relprod

        std::vector<int> effVars; //FD Index of eff variables. Must be sorted!!
        BDD existsVars, existsBwVars;   // Cube with variables to existentialize
        std::vector<BDD> swapVarsS, swapVarsSp; // Swap variables s to sp and viceversa
        std::vector<BDD> swapVarsA, swapVarsAp; // Swap abstraction variables

        std::set<OperatorID> ops; //List of operators represented by the TR

    public:
        //Constructor for transitions irrelevant for the abstraction
        TransitionRelation(SymVariables *sVars, const OperatorProxy &op, int cost_);

        BDD image(bool fw, const BDD &from) const{
            if (fw) {
                return image(from);
            } else {
                return preimage(from);
            }
        }
        BDD image(const BDD &from) const;
        BDD preimage(const BDD &from) const;

        BDD image(bool fw, const BDD &from, long maxNodes) const{
            if (fw) {
                return image(from, maxNodes);
            } else {
                return preimage(from, maxNodes);
            }
        }
        BDD image(const BDD &from, long maxNodes) const;
        BDD preimage(const BDD &from, long maxNodes) const;

        void edeletion(const std::vector<std::vector<BDD>> &notMutexBDDsByFluentFw,
                       const std::vector<std::vector<BDD>> &notMutexBDDsByFluentBw,
                       const std::vector<std::vector<BDD>> &exactlyOneBDDsByFluent);

        void merge(const TransitionRelation &t2, long maxNodes);

        inline int getCost() const {
            return cost;
        }

        inline int nodeCount() const {
            return tBDD.nodeCount();
        }

        inline const std::set<OperatorID> &getOps() const {
            return ops;
        }

        friend std::ostream &operator<<(std::ostream &os, const TransitionRelation &tr);

    };

    TransitionRelation mergeTR(TransitionRelation tr, const TransitionRelation &tr2, int maxSize);

    std::shared_ptr<TransitionRelation>
    merge_uniqueTR(const std::shared_ptr<TransitionRelation>& tr, const std::shared_ptr<TransitionRelation> &tr2, int maxSize);

}
#endif
