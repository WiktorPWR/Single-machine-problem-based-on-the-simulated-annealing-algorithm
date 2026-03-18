#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <thread>
#include <mutex>
#include <algorithm>
#include "config.h"
#include "task.h"
#include "task_generator.h"
#include "simulated_annealing.h"
#include "thread_function.h"

extern std::mutex result_mutex;
extern std::vector<int> best_costs;

extern const int NUM_TASKS;
extern const int NUM_THREADS;

#endif