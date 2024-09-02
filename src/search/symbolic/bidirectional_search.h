#ifndef SYMBOLIC_BIDIRECTIONAL_SEARCH_H
#define SYMBOLIC_BIDIRECTIONAL_SEARCH_H

#include "sym_search.h"
#include "unidirectional_search.h"

namespace symbolic {
    class BidirectionalSearch : public SymSearch {
    private:
        std::unique_ptr<UnidirectionalSearch> fw, bw;

        //Returns the best direction to search the bd exp
        UnidirectionalSearch *selectBestDirection() const;

    public:
        BidirectionalSearch(SymController *eng, const SymParamsSearch &params,
                            std::unique_ptr<UnidirectionalSearch> fw, std::unique_ptr<UnidirectionalSearch> bw);


        virtual bool finished() const override;

        virtual bool stepImage(utils::Duration maxTime, long maxNodes) override;

        //Prints useful statistics at the end of the search
        virtual void statistics() const override;

        virtual int getF() const override {
            return std::max<int>(std::max<int>(fw->getF(), bw->getF()),
                                 fw->getG() + bw->getG() + mgr->getAbsoluteMinTransitionCost());
        }


        virtual bool isSearchableWithNodes(int maxNodes) const override {
            return fw->isSearchableWithNodes(maxNodes) || bw->isSearchableWithNodes(maxNodes);
        }

        virtual utils::Duration nextStepTime() const override {
            return utils::Duration(std::min<double>(fw->nextStepTime(), bw->nextStepTime()));
        }

        virtual long nextStepNodes() const override {
            return std::min<long>(fw->nextStepNodes(), bw->nextStepNodes());
        }

        bool isExpFor(BidirectionalSearch *bdExp) const;

        inline UnidirectionalSearch *getFw() const {
            return fw.get();
        }

        inline UnidirectionalSearch *getBw() const {
            return bw.get();
        }

        friend std::ostream &operator<<(std::ostream &os, const BidirectionalSearch &other);
    };
}
#endif
