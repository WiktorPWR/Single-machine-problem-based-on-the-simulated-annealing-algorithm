#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include <vector>
#include "task.h"
#include <cstdlib>

const int MAX_ITERATIONS = 10;
const double INITIAL_TEMPERATURE = 100.0;
const double COOLING_RATE = 0.95;
const double MIN_TEMPERATURE = 0.1;

int evaluate_solution(std::vector<Task>& tasks);

std::vector<Task> generate_neighbor(std::vector<Task>& current_solution);


#endif