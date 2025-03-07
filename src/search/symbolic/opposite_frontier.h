#ifndef SYMBOLIC_OPPOSITE_FRONTIER_H
#define SYMBOLIC_OPPOSITE_FRONTIER_H

#include "sym_state_space_manager.h"
#include <memory>

#include "closed_list.h"

namespace symbolic {
    class SymSolution;
    class ClosedList;

    class OppositeFrontier {
    public:
        virtual ~OppositeFrontier() = default;

        virtual std::optional<SymSolution> checkCut(const std::shared_ptr<ClosedList> & closed, const BDD &states, int g, bool fw) const = 0;

        virtual BDD notClosed() const = 0;

        //Returns true only if all not closed states are guaranteed to be dead ends
/*
        virtual bool exhausted() const = 0;
*/
        virtual int getGNotClosed() const = 0;
    };

    class OppositeFrontierFixed : public OppositeFrontier {
        std::shared_ptr<SymStateSpaceManager> mgr;
        BDD goal;
        int hNotGoal;
    public:
        OppositeFrontierFixed(const std::shared_ptr<SymStateSpaceManager> & mgr, bool fw);
        ~OppositeFrontierFixed() override = default;

        std::optional<SymSolution> checkCut(const std::shared_ptr<ClosedList> & closed, const BDD &states, int g, bool fw) const override;

        BDD notClosed() const override;

        int getGNotClosed() const override;

    };

    class OppositeFrontierNonStop : public OppositeFrontier {
        BDD goal;
        int hNotGoal;
    public:
        OppositeFrontierNonStop(const std::shared_ptr<SymStateSpaceManager> & mgr);
        ~OppositeFrontierNonStop() override = default;

        std::optional<SymSolution> checkCut(const std::shared_ptr<ClosedList> & closed, const BDD &states, int g, bool fw) const override;

        BDD notClosed() const override {
            return goal;
        }

        int getGNotClosed() const override {
            return hNotGoal;
        }
    };


    class OppositeFrontierClosed : public OppositeFrontier {
        std::shared_ptr<ClosedList> closed;
    public:
        OppositeFrontierClosed(std::shared_ptr<ClosedList> closed) : closed(closed) {}
        ~OppositeFrontierClosed() override = default;

        std::optional<SymSolution> checkCut(const std::shared_ptr<ClosedList> & closed, const BDD &states, int g, bool fw) const override;
        BDD notClosed() const override;
        int getGNotClosed() const override;
    };

}
#endif
