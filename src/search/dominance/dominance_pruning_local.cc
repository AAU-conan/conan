#include "dominance_pruning_local.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"
#include "../task_proxy.h"
#include "../factored_transition_system/factored_state_mapping.h"

using namespace std;

namespace dominance {
    DominancePruningLocal::DominancePruningLocal(bool compare_initial_state, bool compare_siblings,
                                                 const std::shared_ptr<fts::FTSTaskFactory> & fts_factory,
                                                 std::shared_ptr<DominanceAnalysis> dominance_analysis,
                                                 utils::Verbosity verbosity) :
            DominancePruning(fts_factory, dominance_analysis, verbosity),
            compare_initial_state(compare_initial_state),
            compare_siblings (compare_siblings) {
    }

    void DominancePruningLocal::initialize(const std::shared_ptr<AbstractTask> &task) {
        DominancePruning::initialize(task);
        if (compare_initial_state) {
            //TODO (feature): Store representation for initial state
        }
    }


    bool DominancePruningLocal::must_prune_operator(const OperatorProxy & op,
                                                    const State & state,
                                                    const vector<int> & parent,
                                                    const vector<int> & parent_transformed,
                                                    vector<int> & succ,
                                                    vector<int> & succ_transformed) const {
        // TODO (efficiency): Probably it's more efficient to use some kind of flat set for affected_factors based on a sorted vector
        // given that we insert very few elements
        vector<int> updated_variables;
        for (EffectProxy effect : op.get_effects()) {
            if (does_fire(effect, state)) {
                FactPair effect_fact = effect.get_fact().get_pair();
                succ[effect_fact.var] = effect_fact.value;
                updated_variables.push_back(effect_fact.var);
            }
        }
        assert(state_mapping);
        auto maybe_affected_factors = state_mapping->update_transformation_in_place(succ_transformed, succ, updated_variables);

        bool must_prune = false;
        if(!maybe_affected_factors.has_value()) {
            must_prune = true;
        } else{
            // Is dominated by parent?
            must_prune = std::ranges::all_of(maybe_affected_factors.value(),
                                             [&](const auto & factor) {
                                                 return (*dominance_relation)[factor].simulates(parent_transformed[factor],
                                                                                                succ_transformed[factor]);
                                             });
        }

        //TODO (efficiency): update only affected_variables and factors for efficiency reasons?
        succ = parent;
        succ_transformed = parent_transformed;
        return must_prune;
    }


    void DominancePruningLocal::prune(const State &state, std::vector<OperatorID> &op_ids) {
        state.unpack();
        vector<int> parent = state.get_unpacked_values();
        vector<int> parent_transformed = state_mapping->transform(parent);

        vector<int> succ = parent;
        vector<int> succ_transformed = parent_transformed;

        TaskProxy tp (*task);

        const auto [first, last] = std::ranges::remove_if(op_ids,
                                                          [&] (const auto & op_id) {
            return must_prune_operator(tp.get_operators()[op_id], state, parent, parent_transformed, succ, succ_transformed);
        });

        op_ids.erase(first, last);
    }


    class DominancePruningLocalFeature
            : public plugins::TypedFeature<PruningMethod, DominancePruningLocal> {
    public:
        DominancePruningLocalFeature() : TypedFeature("dominance_local") {
            document_title("Local dominance pruning");

            document_synopsis(
                    "This pruning method implements the algorithm described in the following "
                    "paper:" + utils::format_conference_reference(
                            {"{\'A}lvaro Torralba", "J\"org Hoffmann"},
                            "Simulation-Based Admissible Dominance Pruning",
                            "https://homes.cs.aau.dk/~alto/papers/ijcai15.pdf",
                            "Proceedings of the 24th International Joint Conference on Artificial Intelligence (IJCAI'15)",
                            "1689-1695",
                            "AAAI Press",
                            "2015") + "\n"//TODO (doc): Reference other relevant papers
            );
            document_language_support("action costs", "supported");
            document_language_support("conditional effects", "not supported");
            document_language_support("axioms", "not supported");

            document_property("admissible", "yes");
            document_property("safe", "yes");


            document_synopsis(
                    "Compares states against a few states around it.");

            add_option<bool>(
                    "compare_initial_state",
                    "Whether to compare all states against the initial state",
                    "true");

            add_option<bool>(
                    "compare_siblings",
                    "Whether to compare all states against their siblings",
                    "true");

            add_dominance_pruning_options_to_feature(*this);


            // TODO (documentation): add example
            document_note(
                    "Example",
                    "{{{\npruning=dominance_local()\n}}}\n"
                    "in an eager search such as astar.");
        }


        virtual shared_ptr <DominancePruningLocal> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {

            return plugins::make_shared_from_arg_tuples<DominancePruningLocal>(
                    opts.get<bool>("compare_initial_state"),
                    opts.get<bool>("compare_siblings"),
                    get_dominance_pruning_arguments_from_options(opts));
        }
    };

    static plugins::FeaturePlugin<DominancePruningLocalFeature> _plugin;

}