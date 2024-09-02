#include "bucket.h"

#include <cassert>

using namespace std;

namespace symbolic {

    bool extract_states(DisjunctiveBucket &list,
                        const DisjunctiveBucket &pruned,
                        DisjunctiveBucket &res) {
        assert(!pruned.bucket.empty());

        bool somethingPruned = false;
        for (auto &bddList: list.bucket) {
            BDD prun = pruned.bucket[0] * bddList;
            //TODO: redundant operation
            for (const auto &prbdd: pruned.bucket) {
                prun += prbdd * bddList;
            }

            if (!prun.IsZero()) {
                somethingPruned = true;
                bddList -= prun;
                res.bucket.push_back(prun);
            }
        }
        list.clean();
        return somethingPruned;
    }

    void copyBucket(const DisjunctiveBucket &bucket, DisjunctiveBucket &res) {
        if (!bucket.bucket.empty()) {
            res.bucket.insert(std::end(res.bucket), std::begin(bucket.bucket), std::end(bucket.bucket));
        }
    }

    void moveBucket(DisjunctiveBucket &bucket, DisjunctiveBucket &res) {
        copyBucket(bucket, res);
        bucket.reset();
    }
}