#ifndef DOMINANCE_LD_SIMULATION_H
#define DOMINANCE_LD_SIMULATION_H

#include <fstream>
#include <vector>
#include <ostream>
#include "qualified_local_state_relation2.h"
#include "qualified_factored_dominance_relation.h"
#include "../factored_transition_system/labelled_transition_system.h"
#include "../factored_transition_system/fts_task.h"
#include "../utils/logging.h"
#include "../dominance/all_none_factor_index.h"

#include "qualified_dominance_analysis.h"
#include "../utils/timer.h"
#include "../utils/logging.h"


namespace fts {
    class FTSTask;
}

namespace qdominance {
    class QualifiedLDSimulation : public QualifiedDominanceAnalysis {
        utils::LogProxy log;
    public:
        explicit QualifiedLDSimulation(utils::Verbosity verbosity);

        virtual ~QualifiedLDSimulation() = default;
        std::unique_ptr<QualifiedFactoredDominanceRelation> compute_dominance_relation(const fts::FTSTask &task) override;
    };



    template<typename LR>
    std::unique_ptr<QualifiedFactoredDominanceRelation> compute_ld_simulation(const fts::FTSTask & task, utils::LogProxy & log) {
        utils::Timer t;

        LR label_relation {task};

        std::vector<std::unique_ptr<QualifiedLocalStateRelation2>> local_relations;
        local_relations.reserve(task.get_num_variables());

        for (const auto & [factor, lts]: std::views::enumerate(task.get_factors())) {
            local_relations.push_back(std::make_unique<QualifiedLocalStateRelation2>(*lts, factor, task, label_relation));
        }

        log << "Initialize qualified label dominance: " << task.get_num_labels() << " labels " << task.get_num_variables() << " systems." << std::endl;


        int total_size = 0, max_size = 0, total_trsize = 0, max_trsize = 0;
        for (const auto & lts: task.get_factors()) {
            max_size = std::max(max_size, lts->size());
            max_trsize = std::max(max_trsize, lts->num_transitions());
            total_size += lts->size();
            total_trsize += lts->num_transitions();
        }
        log << "Compute LDSim on " << task.get_num_variables() << " LTSs."
                  << " Total factor size: " << total_size
                  << ", total trsize: " << total_trsize
                  << ", max factor size: " << max_size
                  << ", max trsize: " << max_trsize
                  << std::endl;

        log << "Init LDSim in " << t() << ":" << std::endl << std::flush;
        do {
            for (const auto & local_relation : local_relations) {
                // log << "Updating local relation " << i << std::endl;
                local_relation->update(label_relation);
            }
            log << " " << t() << std::flush;
        } while (label_relation.update(local_relations));
        log << std::endl << "LDSimulation finished: " << t() << std::endl;


        return std::make_unique<QualifiedFactoredDominanceRelation>(std::move(local_relations));
    }
}

#endif
