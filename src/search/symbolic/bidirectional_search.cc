#include "bidirectional_search.h"

#include "sym_controller.h"

#include <algorithm>    // std::reverse
#include <memory>

using namespace std;

namespace symbolic {
    BidirectionalSearch::BidirectionalSearch(const SymParamsSearch &params,
                                             std::unique_ptr<SymSearch> _fw,
                                             unique_ptr <SymSearch> _bw) :
            SymSearch(params, _fw->getStateSpaceShared(), _fw->getSolutionShared()),
            fw(std::move(_fw)),
            bw(std::move(_bw)) {

        assert(mgr == bw->getStateSpaceShared());
        assert(solution == bw->getSolutionShared());
    }

    SymSearch * BidirectionalSearch::selectBestDirection() const {
        bool fwSearchable = fw->isSearchable();
        bool bwSearchable = bw->isSearchable();
        if (fwSearchable && !bwSearchable) {
            return fw.get();
        } else if (!fwSearchable && bwSearchable) {
            return bw.get();
        }
        return fw->nextStepNodes() <= bw->nextStepNodes() ? fw.get() : bw.get();
    }

    bool BidirectionalSearch::finished() const {
        return fw->finished() || bw->finished();
    }

    void BidirectionalSearch::statistics() const {
        if (fw) {
            fw->statistics();
        }
        if (bw) {
            bw->statistics();
        }
        cout << endl;
    }

    int BidirectionalSearch::getF() const {
        return std::max<int>(std::max<int>(fw->getF(), bw->getF()),
                             fw->getG() + bw->getG() + mgr->getAbsoluteMinTransitionCost());
    }

    bool BidirectionalSearch::stepImage(utils::Duration maxTime, long maxNodes) {
        bool res = selectBestDirection()->stepImage(maxTime, maxNodes);
        solution->setLowerBound(getF(), p.log);
        return res;
    }
}
