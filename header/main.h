#ifndef MAIN_H
#define MAIN_H

#include <iostream>
#include <fstream> // Potrzebne do zapisu pliku
#include <vector>
#include <cmath>
#include "task.h"
#include "task_generator.h"
#include "simulated_annealing.h"
#include <thread>
#include "thread_function.h"
#include <mutex>
#include <algorithm> // dla std::min_element

extern std::mutex result_mutex; 
extern std::vector<int> best_costs;

#endif