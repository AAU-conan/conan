#ifndef FTS_FACTORED_STATE_MAPPING_H
#define FTS_FACTORED_STATE_MAPPING_H

#include <vector>
#include <memory>
#include <optional>

#include "../task_proxy.h"

namespace merge_and_shrink {
    class MergeAndShrinkRepresentation;
}

class State;
namespace fts {
    class FactoredStateMapping {
        public:
            virtual ~FactoredStateMapping() = default;

            virtual int get_value(const std::vector<int> & state, int factor) = 0;

            virtual std::vector<int> transform(const std::vector<int> & state) = 0;

            virtual std::optional<std::vector<int>> update_transformation_in_place(std::vector<int> & transformed_state_values,
                                                                    const std::vector<int> & state_values,
                                                                    std::vector<int> & updated_state_variables)  = 0;
    };

    class FactoredStateMappingIdentity : public FactoredStateMapping {
    public:
        virtual ~FactoredStateMappingIdentity() = default;

        virtual int get_value(const std::vector<int> & state, int factor) override;
        virtual std::vector<int> transform(const std::vector<int> & state) override;

        virtual std::optional<std::vector<int>> update_transformation_in_place(std::vector<int> & transformed_state_values,
                                                        const std::vector<int> & state_values,
                                                        std::vector<int> & updated_state_variables) override;
    };

    class FactoredStateMappingMergeAndShrink : public FactoredStateMapping {
        std::vector<std::unique_ptr<merge_and_shrink::MergeAndShrinkRepresentation>> factored_mapping;
        std::vector<int> variable_to_factor;

    public:
        virtual ~FactoredStateMappingMergeAndShrink() = default;

        virtual int get_value(const std::vector<int> & state, int factor) override;
        virtual std::vector<int> transform(const std::vector<int> & state) override;

        virtual std::optional<std::vector<int>> update_transformation_in_place(std::vector<int> & transformed_state_values,
                                                        const std::vector<int> & state_values,
                                                        std::vector<int> & updated_state_variables) override;

        const auto & get_mapping() const {
            return factored_mapping;
        }

    };

}
#endif
