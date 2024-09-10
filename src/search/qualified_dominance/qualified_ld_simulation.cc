#include "qualified_ld_simulation.h"

#include "../factored_transition_system/fts_task.h"
#include "qualified_label_relation.h"

#include "../plugins/plugin.h"
#include "../utils/markup.h"

using std::vector;
using merge_and_shrink::TransitionSystem;

using namespace dominance;

namespace qdominance {
    std::unique_ptr<QualifiedFactoredDominanceRelation> QualifiedLDSimulation::compute_dominance_relation(const fts::FTSTask &task) {
        return compute_ld_simulation<QualifiedLabelRelation>(task, log);
    }

    QualifiedLDSimulation::QualifiedLDSimulation(utils::Verbosity verbosity) :
            log(utils::get_log_for_verbosity(verbosity)) {
    }


    class LDSimulationFeature
            : public plugins::TypedFeature<QualifiedDominanceAnalysis, QualifiedLDSimulation> {
    public:
        LDSimulationFeature() : TypedFeature("qld_simulation") {
            document_title("LDSimulation");

            document_synopsis(
                    "This dominance analysis method implements the algorithm described in the following "
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

            utils::add_log_options_to_feature(*this);
        }


        virtual std::shared_ptr<QualifiedLDSimulation> create_component(
                const plugins::Options &opts,
                const utils::Context &) const override {

            return plugins::make_shared_from_arg_tuples<QualifiedLDSimulation>(
                    utils::get_log_arguments_from_options(opts));
        }
    };

static plugins::FeaturePlugin<LDSimulationFeature> _plugin;

}