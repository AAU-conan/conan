#include "all_none_factor_index.h"

std::string to_string(dominance::AllNoneFactorIndex a) {
    if (a.is_all())
        return "all";
    if (a.is_none())
        return "none";
    return std::to_string(a.get_not_present_factor());
}

namespace dominance {

}