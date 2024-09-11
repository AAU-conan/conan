#ifndef PRUNING_LIMITED_PRUNING_H
#define PRUNING_LIMITED_PRUNING_H

#include "../pruning_method.h"

namespace plugins {
class Options;
}

namespace limited_pruning {
class LimitedPruning : public PruningMethod {
    std::shared_ptr<PruningMethod> pruning_method;
    const double min_required_pruning_ratio;
    const int num_expansions_before_checking_pruning_ratio;
    int num_pruning_calls;
    bool is_pruning_disabled;

    virtual void prune_generation(
        const State &state, const SearchNodeInfo & node_info, std::vector<OperatorID> &op_ids) override;

    virtual bool prune_expansion(
            const State &state, const SearchNodeInfo & node_info) override;
public:
    explicit LimitedPruning(
        const std::shared_ptr<PruningMethod> &pruning,
        double min_required_pruning_ratio,
        int expansions_before_checking_pruning_ratio,
        utils::Verbosity verbosity);
    virtual void initialize(const std::shared_ptr<AbstractTask> &) override;
};
}

#endif
