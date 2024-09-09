#ifndef DOMINANCE_DOMINANCE_PRUNING_DD_H
#define DOMINANCE_DOMINANCE_PRUNING_DD_H

#include "dominance_pruning.h"
#include "../symbolic/bdd_manager.h"
#include "../symbolic/sym_variables.h"

namespace fts {
    class FactoredSymbolicStateMapping;
    class SymbolicStateMapping;
}
namespace dominance {
    enum class PruningDD {
        BDD_MAP, ADD, BDD, BDD_MAP_DISJ, SKYLINE_BDD_MAP, SKYLINE_BDD
    };

    std::ostream &operator<<(std::ostream &os, const PruningDD &m);

    class LocalStateRelationBDD {
        std::vector<BDD> dominance_bdds;  //For each abstract state, we create a BDD that represents all the abstract states dominated by it or dominating it

    public:
        LocalStateRelationBDD(std::vector<BDD> && dominance_bdds) :
                dominance_bdds(std::move(dominance_bdds)) {}

        BDD get_dominance_bdd(int value) const;

        static std::unique_ptr<LocalStateRelationBDD> precompute_dominating_bdds(const fts::SymbolicStateMapping & symbolic_mapping,
                                                                                 const LocalStateRelation & state_relation);

        static std::unique_ptr<LocalStateRelationBDD> precompute_dominated_bdds(const fts::SymbolicStateMapping & symbolic_mapping,
                                                                                const LocalStateRelation & state_relation);
    };

    class DominancePruningDD : public DominancePruning {
    protected:
        std::shared_ptr <symbolic::SymVariables> vars; //The symbolic variables are declared here
        const bool remove_spurious_dominated_states;
        const bool insert_dominated;

        std::vector<std::unique_ptr<LocalStateRelationBDD>> local_bdd_representation;

        BDD getBDDToInsert(const State &state);

        BDD getIrrelevantStates() const;

        //Methods to keep dominated states in explicit search
        //Check: returns true if a better or equal state is known
        virtual bool check(const State &state, int g) = 0;
        virtual void insert(const State &state, int g) = 0;

    public:
        DominancePruningDD(bool insert_dominated, bool remove_spurious_dominated_states);
        virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
        virtual void prune(const State &state, std::vector<OperatorID> &op_ids) override;
    };


    class DominancePruningBDDMap : public DominancePruningDD {
        std::map<int, BDD> closed;
    public:
        DominancePruningBDDMap(bool insert_dominated, bool remove_spurious_dominated_states) : DominancePruningDD(insert_dominated, remove_spurious_dominated_states) {}

        virtual ~DominancePruningBDDMap() = default;

        //Methods to keep dominated states in explicit search
        virtual bool check(const State &state, int g) override;

        virtual void insert(const State &state, int g) override;
    };

    class DominancePruningBDDMapDisj : public DominancePruningDD {
        std::map<int, std::vector<BDD> > closed;
    public:
        DominancePruningBDDMapDisj(bool insert_dominated, bool remove_spurious_dominated_states) :
                DominancePruningDD(insert_dominated, remove_spurious_dominated_states) {}

        virtual ~DominancePruningBDDMapDisj()=default;

        //Methods to keep dominated states in explicit search
        virtual bool check(const State &state, int g) override;

        virtual void insert(const State &state, int g) override;
    };


    class DominancePruningBDD : public DominancePruningDD {
        BDD closed, closed_inserted;
        bool initialized;
    public:
        DominancePruningBDD(bool insert_dominated, bool remove_spurious_dominated_states) :
                DominancePruningDD(insert_dominated, remove_spurious_dominated_states), initialized(false) {}

        virtual ~DominancePruningBDD() {}

        //Methods to keep dominated states in explicit search
        virtual bool check(const State &state, int g) override;

        virtual void insert(const State &state, int g) override;
    };

    class DominancePruningSkylineBDDMap : public DominancePruningDD {
        // We have the set of states inserted with each g-value that could dominate each fluent
        std::vector<std::vector<std::map<int, BDD> > > closed;
        std::set<int> g_values;
    public:
        DominancePruningSkylineBDDMap(bool insert_dominated, bool remove_spurious_dominated_states) :
                DominancePruningDD(insert_dominated, remove_spurious_dominated_states) {}

        virtual ~DominancePruningSkylineBDDMap() = default;

        //Methods to keep dominated states in explicit search
        virtual bool check(const State &state, int g) override;

        virtual void insert(const State &state, int g) override;
    };

    class DominancePruningSkylineBDD : public DominancePruningDD {
        std::vector<std::vector<BDD> > closed;
    public:
        DominancePruningSkylineBDD(bool insert_dominated, bool remove_spurious_dominated_states) :
                DominancePruningDD(insert_dominated, remove_spurious_dominated_states) {}

        virtual ~DominancePruningSkylineBDD() = default;

        //Methods to keep dominated states in explicit search
        virtual bool check(const State &state, int g) override;

        virtual void insert(const State &state, int g) override;
    };

}

#endif
