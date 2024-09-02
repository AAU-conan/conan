#ifndef SYMBOLIC_SYM_BUCKET_H
#define SYMBOLIC_SYM_BUCKET_H

#include <vector>
#include "cuddObj.hh"
#include "bdd_manager.h"
#include "merge_bdds.h"

namespace symbolic {

    class DisjunctiveBucket {
    public:
        std::vector<BDD> bucket;

        DisjunctiveBucket() = default;

        explicit DisjunctiveBucket(const std::vector<BDD> &bucket) : bucket(bucket) {}

        bool empty() const { //TODO:Perhaps for conjunctive buckets this should be different
            return bucket.empty();
        }

        void clean() {
            bucket.erase(remove_if(std::begin(bucket), std::end(bucket),
                                   [](BDD &bdd) { return bdd.IsZero(); }),
                         std::end(bucket));
        }

        int nodeCount() {
            int sum = 0;
            for (const BDD &bdd: bucket) {
                sum += bdd.nodeCount();
            }
            return sum;
        }

        void reset() {
            std::vector<BDD>().swap(bucket);
        }

        bool merge_bucket(BDDManager *mgr, utils::Duration maxTime, long maxNodes) {
            merge(mgr, bucket, mergeOrBDD, maxTime, maxNodes);
            clean();
            return bucket.size() <= 1;
        }

        void push_back(BDD bdd) {
            bucket.push_back(bdd);
        }

    };

    bool extract_states(DisjunctiveBucket &list, const DisjunctiveBucket &pruned, DisjunctiveBucket &res);

    void copyBucket(const DisjunctiveBucket &bucket, DisjunctiveBucket &res);

    void moveBucket(DisjunctiveBucket &bucket, DisjunctiveBucket &res);

}

#endif
