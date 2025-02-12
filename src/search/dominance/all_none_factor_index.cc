#include "all_none_factor_index.h"

#include <format>

std::string to_string(dominance::AllNoneFactorIndex a) {
    if (a.is_all())
        return "all";
    if (a.is_none())
        return "none";
    return std::format("all_except_{}", a.get_not_present_factor());
}

namespace dominance {

}