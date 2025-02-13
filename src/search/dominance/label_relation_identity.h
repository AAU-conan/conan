#ifndef DOMINANCE_LABEL_RELATION_IDENTITY_H
#define DOMINANCE_LABEL_RELATION_IDENTITY_H

#include <iostream>
#include <vector>
#include "label_relation.h"

class EquivalenceRelation;
namespace dominance {
    class LabelledTransitionSystem;
    class DominanceRelation;
/*
 * Label relation represents the preorder relations on labels that
 * occur in a set of LTS
 */
    class LabelRelationIdentity {
        Labels *labels;
        int num_labels;

    public:
        LabelRelationIdentity(Labels *labels,
                              const std::vector<LabelledTransitionSystem *> & /*lts*/,
                              const DominanceRelation & /*sim*/, const LabelMap & /*labelMap*/) {}

        bool update(const std::vector<LabelledTransitionSystem *> & /*lts*/,
                    const DominanceRelation & /*sim*/) { return false; }

        void dump() const {}
        void dump(int /*label*/) const {}
        void dump_equivalent() const {}
        void dump_dominance() const {}


        inline int get_num_labels() const {
            return num_labels;
        }

        inline int get_dominated_by_noop_in(int /*l*/) const {
            return DOMINATES_IN_NONE;
        }

        inline bool dominated_by_noop(int /*l*/, int /*lts*/) const {
            return false;
        }

        inline bool dominates(int l1, int l2, int /*lts*/) const {
            return l1 == l2;
        }

        std::vector<int> get_labels_dominated_in_all() const {
            return std::vector<int>();
        }
    };
}
    

#endif
