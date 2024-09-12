#ifndef DOMINANCE_ALL_NONE_FACTOR_INDEX_H
#define DOMINANCE_ALL_NONE_FACTOR_INDEX_H

#include <cassert>
#include <string>

namespace dominance {
    // Represents whether l1 dominates l2 in all, all except "factor", or none
    class AllNoneFactorIndex {
        static const int DOMINATES_IN_ALL = -2;
        static const int DOMINATES_IN_NONE = -1;

        int not_present_factor;

    public:
        AllNoneFactorIndex (int not_present_factor) :
            not_present_factor (not_present_factor) {
        }
        static AllNoneFactorIndex all_factors() {
            return AllNoneFactorIndex (DOMINATES_IN_ALL);
        }
        static AllNoneFactorIndex no_factors() {
            return AllNoneFactorIndex (DOMINATES_IN_NONE);
        }

        bool is_none() const {
            return not_present_factor == DOMINATES_IN_NONE;
        }

        bool is_all() const {
            return not_present_factor == DOMINATES_IN_ALL;
        }

        bool is_factor() const {
            return not_present_factor >= 0;
        }

        int get_not_present_factor () const {
            assert(is_factor());
            return not_present_factor;
        }

        bool contains (int factor) const {
            return !is_none() && (is_all() || not_present_factor != factor);
        }

        bool remove (int factor) {
            if (is_all()) {
                not_present_factor = factor;
                return true;
            } else if (!is_none() && not_present_factor != factor) {
                not_present_factor = DOMINATES_IN_NONE;
                return true;
            }
            return false;
        }

        bool contains_all_except (int factor) const {
            return is_all() || factor == not_present_factor;
        }

        bool operator ==(const AllNoneFactorIndex & other) const {
            return not_present_factor == other.not_present_factor;
        }

    };

}

std::string to_string(dominance::AllNoneFactorIndex a);


#endif
