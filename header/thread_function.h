#ifndef THREAD_FUNCTION_H
#define THREAD_FUNCTION_H

#include "task.h"
#include "simulated_annealing.h"
#include <vector>
#include <mutex>
#include <iostream>

extern std::mutex result_mutex;
extern std::vector<int> best_costs;

void thread_simulated_annealing_function(std::vector<Task>& tasks, int thread_id);

#endif