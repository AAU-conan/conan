#ifndef SYMBOLIC_BIDIRECTIONAL_SEARCH_H
#define SYMBOLIC_BIDIRECTIONAL_SEARCH_H

#include "sym_search.h"

namespace symbolic {
    class BidirectionalSearch : public SymSearch {
    private:
        std::unique_ptr<SymSearch> fw, bw;

        //Returns the best direction to search the bd exp
        SymSearch *selectBestDirection() const;

    public:
        BidirectionalSearch(const SymParamsSearch &params,
                            std::unique_ptr<SymSearch> fw, std::unique_ptr<SymSearch> bw);


        bool finished() const override;

        bool stepImage(utils::Duration maxTime, long maxNodes) override;

        //Prints useful statistics at the end of the search
        void statistics() const override;

        int getF() const override;


        bool isSearchableWithNodes(int maxNodes) const override {
            return fw->isSearchableWithNodes(maxNodes) || bw->isSearchableWithNodes(maxNodes);
        }

        int getG() const override {
            return fw->getG() + bw->getG();
        }

        utils::Duration nextStepTime() const override {
            return utils::Duration(std::min<double>(fw->nextStepTime(), bw->nextStepTime()));
        }

        long nextStepNodes() const override {
            return std::min<long>(fw->nextStepNodes(), bw->nextStepNodes());
        }

        bool isExpFor(BidirectionalSearch *bdExp) const;

        inline SymSearch *getFw() const {
            return fw.get();
        }

        inline SymSearch *getBw() const {
            return bw.get();
        }

        friend std::ostream &operator<<(std::ostream &os, const BidirectionalSearch &other);
    };
}
#endif