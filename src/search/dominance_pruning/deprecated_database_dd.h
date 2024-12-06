#ifndef DOMINANCE_DOMINANCE_PRUNING_DD_H
#define DOMINANCE_DOMINANCE_PRUNING_DD_H

#include "dominance_pruning_previous.h"

#include "../symbolic/bdd_manager.h"
#include "../symbolic/sym_variables.h"
#include "../dominance/dominance_analysis.h"

namespace fts {
    class FactoredSymbolicStateMapping;

    class SymbolicStateMapping;
}

namespace dominance {

    class DatabaseDD : public DominanceDatabase {
    protected:
        std::shared_ptr<symbolic::SymVariables> vars;
    };

    class DatabaseDDFactory : public DominanceDatabaseFactory {
        const std::shared_ptr<symbolic::BDDManager> bdd_mgr;
        std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy;

    public:
        DatabaseDDFactory(std::shared_ptr<symbolic::BDDManager> bdd_mgr, std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy, utils::Verbosity verbosity);

        std::unique_ptr<DominanceDatabase> create(std::shared_ptr<AbstractTask> &task,
            std::unique_ptr<FactoredDominanceRelation> dominance_relation,
            std::shared_ptr<fts::FactoredStateMapping> state_mapping) override;
    };



    // class DominancePruningBDDMapDisj : public DominancePruningDD {
    //     std::map<int, std::vector<BDD> > closed;
    // public:
    //     DominancePruningBDDMapDisj(bool insert_dominated, bool remove_spurious_dominated_states,
    //                                const symbolic::BDDManagerParameters &bdd_mgr_params, std::shared_ptr<variable_ordering::VariableOrderingStrategy> variable_ordering_strategy,
    //                                const std::shared_ptr<fts::FTSTaskFactory> &fts_factory,
    //                                std::shared_ptr<DominanceAnalysis> dominance_analysis,
    //                                utils::Verbosity verbosity) :
    //             DominancePruningDD(insert_dominated, remove_spurious_dominated_states, bdd_mgr_params, variable_ordering_strategy, fts_factory,
    //                                dominance_analysis, verbosity) {}
    //
    //     virtual ~DominancePruningBDDMapDisj() = default;
    //
    //     virtual bool check(const ExplicitState &state, int g) const override;
    //
    //     virtual void insert(const ExplicitState &state, int g) override;
    // };


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
