#ifndef SEARCH_SPACE_DRAW_H
#define SEARCH_SPACE_DRAW_H

#include "../task_proxy.h"


void set_initial_state(const State &initial_state);

void add_successor(const State &state, const OperatorProxy &op, const State &successor_state);

void set_heuristic_value(const State &state, int value);

void draw_search_space(const std::shared_ptr<AbstractTask>& task, const std::string& output_path);

#endif //SEARCH_SPACE_DRAW_H
