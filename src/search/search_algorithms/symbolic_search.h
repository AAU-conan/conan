#ifndef SEARCH_ALGORITHMS_SYMBOLIC_SEARCH_H
#define SEARCH_ALGORITHMS_SYMBOLIC_SEARCH_H

#include <vector>
#include <memory>

#include "../search_algorithm.h"
#include "../symbolic/sym_solution.h"

namespace plugins {
    class Options;
}

namespace symbolic {
    class SymStateSpaceManager;
    class SymSearch;
    class SymSolution;

}

namespace symbolic_search {

    class SymbolicSearch : public SearchAlgorithm, public symbolic::SolutionNotifier {
    protected:
	std::shared_ptr<symbolic::BDDManager> bdd_manager;
	std::shared_ptr<symbolic::SymVariables> vars;
	std::shared_ptr<symbolic::SymStateSpaceManager> mgr;

   	std::shared_ptr<symbolic::SymSolutionLowerBound> solution;

	std::unique_ptr<symbolic::SymSearch> search;

	SearchStatus step() override;

    public:
	SymbolicSearch(const plugins::Options &opts);
	virtual ~SymbolicSearch() = default;

	void notify_solution(const symbolic::SymSolution & sol) override;

    void print_statistics() const override;
    };


    class SymbolicBidirectionalUniformCostSearch : public SymbolicSearch {
    public:
	SymbolicBidirectionalUniformCostSearch(const plugins::Options &opts);
	virtual ~SymbolicBidirectionalUniformCostSearch() = default;
    };

    
    class SymbolicUniformCostSearch : public SymbolicSearch { 
	bool fw;

    public:
	SymbolicUniformCostSearch(const plugins::Options &opts, bool _fw);
	virtual ~SymbolicUniformCostSearch() = default;
    };

    
    
}

#endif
