#ifndef DOMINANCE_DOMINANCE_PRUNING_DD_H
#define DOMINANCE_DOMINANCE_PRUNING_DD_H

#include "dominance_pruning_previous.h"

#include "../symbolic/bdd_manager.h"
#include "../symbolic/sym_variables.h"

namespace fts {
    class FactoredSymbolicStateMapping;

    class SymbolicStateMapping;
}
namespace dominance {
    class LocalStateRelationBDD {
        std::vector<BDD> dominance_bdds;  //For each abstract state, we create a BDD that represents all the abstract states dominated by it or dominating it
    public:
        LocalStateRelationBDD(std::vector<BDD> &&dominance_bdds) :
                dominance_bdds(std::move(dominance_bdds)) {}

        BDD get_dominance_bdd(int value) const;

        static std::unique_ptr<LocalStateRelationBDD>
        precompute_dominating_bdds(const fts::SymbolicStateMapping &symbolic_mapping,
                                   const LocalStateRelation &state_relation);

        static std::unique_ptr<LocalStateRelationBDD>
        precompute_dominated_bdds(const fts::SymbolicStateMapping &symbolic_mapping,
                                  const LocalStateRelation &state_relation);
    };

    class DominanceRelationBDD {
        std::vector<std::unique_ptr<LocalStateRelationBDD>> local_bdd_representation;

        public:
        DominanceRelationBDD (const FactoredDominanceRelation & dominance_relation,
                              const fts::FactoredSymbolicStateMapping &symbolic_mapping) ;

        BDD setOfDominatingStates(const ExplicitState &state) const;
        BDD setOfDominatedStates(const ExplicitState &state) const;
    };

    class DominancePruningDD : public DominancePruningPrevious {
    protected:
        DominancePruningDD(bool insert_dominated, bool remove_spurious_dominated_states,
                           const symbolic::BDDManagerParameters &bdd_mgr_params,
                           std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy,
                           const std::shared_ptr<fts::FTSTaskFactory> &fts_factory,
                           std::shared_ptr<DominanceAnalysis> dominance_analysis, utils::Verbosity verbosity);

        const std::shared_ptr<symbolic::BDDManager> bdd_mgr;
        std::shared_ptr<symbolic::SymVariables> vars; //The symbolic variables are declared here
        std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy;
        const bool insert_dominated;
        const bool remove_spurious_dominated_states;

        std::unique_ptr<DominanceRelationBDD> dominance_relation_bdd;
        //Task Dependent attrbitutes (they should go into another class once we get rid of initialize and separate this into factory and taskdependent classes).

        BDD getBDDToInsert(const ExplicitState &state) const;
    public:
        virtual void initialize(const std::shared_ptr<AbstractTask> &task) override;
    };


    class DominancePruningBDDMap : public DominancePruningDD {
        std::map<int, BDD> closed;
    public:
        DominancePruningBDDMap(bool insert_dominated, bool remove_spurious_dominated_states,
                               const symbolic::BDDManagerParameters &bdd_mgr_params, std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy,
                               const std::shared_ptr<fts::FTSTaskFactory> &fts_factory,
                               std::shared_ptr<DominanceAnalysis> dominance_analysis,
                               utils::Verbosity verbosity) :
                DominancePruningDD(insert_dominated, remove_spurious_dominated_states, bdd_mgr_params, variable_ordering_strategy, fts_factory,
                                   dominance_analysis, verbosity) {}

        virtual ~DominancePruningBDDMap() = default;

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };

    class DominancePruningBDDMapDisj : public DominancePruningDD {
        std::map<int, std::vector<BDD> > closed;
    public:
        DominancePruningBDDMapDisj(bool insert_dominated, bool remove_spurious_dominated_states,
                                   const symbolic::BDDManagerParameters &bdd_mgr_params, std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy,
                                   const std::shared_ptr<fts::FTSTaskFactory> &fts_factory,
                                   std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                   utils::Verbosity verbosity) :
                DominancePruningDD(insert_dominated, remove_spurious_dominated_states, bdd_mgr_params, variable_ordering_strategy, fts_factory,
                                   dominance_analysis, verbosity) {}

        virtual ~DominancePruningBDDMapDisj() = default;

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };


    /*
    class DominancePruningBDD : public DominancePruningDD {
        BDD closed, closed_inserted;
    public:

        DominancePruningBDD(bool insert_dominated, bool remove_spurious_dominated_states,
                            const symbolic::BDDManagerParameters &bdd_mgr_params, std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy,
                            const std::shared_ptr<fts::FTSTaskFactory> &fts_factory,
                            std::shared_ptr<DominanceAnalysis> dominance_analysis,
                            utils::Verbosity verbosity);

        virtual ~DominancePruningBDD() {}

        virtual bool check(const ExplicitState &state, int g) const override;

        virtual void insert(const ExplicitState &state, int g) override;
    };
    */


//    class DominancePruningSkylineBDDMap : public DominancePruningDD {
//        // We have the set of states inserted with each g-value that could dominate each fluent
//        std::vector<std::vector<std::map<int, BDD> > > closed;
//        std::set<int> g_values;
//    public:
//        DominancePruningSkylineBDDMap(bool insert_dominated, bool remove_spurious_dominated_states) :
//                DominancePruningDD(insert_dominated, remove_spurious_dominated_states) {}
//
//        virtual ~DominancePruningSkylineBDDMap() = default;
//
//        virtual bool check(const ExplicitState &state, int g) const override;
//        virtual void insert(const ExplicitState &state, int g) override;
//    };
//
//    class DominancePruningSkylineBDD : public DominancePruningDD {
//        std::vector<std::vector<BDD> > closed;
//    public:
//        DominancePruningSkylineBDD(bool insert_dominated, bool remove_spurious_dominated_states) :
//                DominancePruningDD(insert_dominated, remove_spurious_dominated_states) {}
//
//        virtual ~DominancePruningSkylineBDD() = default;
//
//        virtual bool check(const ExplicitState &state, int g) const override;
//        virtual void insert(const ExplicitState &state, int g) override;
//    };

}

#endif
