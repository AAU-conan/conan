#ifndef SEARCH_ALGORITHMS_SYMBOLIC_SEARCH_H
#define SEARCH_ALGORITHMS_SYMBOLIC_SEARCH_H

#include <vector>
#include <memory>

#include "../search_algorithm.h"
#include "../symbolic/sym_controller.h"

namespace plugins {
    class Options;
}

namespace symbolic {
    class SymStateSpaceManager;
    class SymSearch;
    class SymSolution;

}

namespace symbolic_search {

    class SymbolicSearch : public SearchAlgorithm, public symbolic::SymController {
    protected:
	// Symbolic manager to perform bdd operations
	std::shared_ptr<symbolic::SymStateSpaceManager> mgr; 

	std::unique_ptr<symbolic::SymSearch> search;

	virtual SearchStatus step() override;

    public:
	SymbolicSearch(const plugins::Options &opts);
	virtual ~SymbolicSearch() = default;

	virtual void new_solution(const symbolic::SymSolution & sol) override;

    virtual void print_statistics() const override;

    };


    class SymbolicBidirectionalUniformCostSearch : public SymbolicSearch { 
    protected:
	virtual void initialize() override;
	
    public:
	SymbolicBidirectionalUniformCostSearch(const plugins::Options &opts);
	virtual ~SymbolicBidirectionalUniformCostSearch() = default;
    };

    
    class SymbolicUniformCostSearch : public SymbolicSearch { 
	bool fw;
    protected:
	virtual void initialize() override;
	
    public:
	SymbolicUniformCostSearch(const plugins::Options &opts, bool _fw);
	virtual ~SymbolicUniformCostSearch() = default;

    };

    
    
}

#endif
