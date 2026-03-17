#ifndef SIMULATED_ANNEALING_H
#define SIMULATED_ANNEALING_H

#include <vector>
#include "task.h"
#include <cstdlib>

const int MAX_ITERATIONS = 10;
const double INITIAL_TEMPERATURE = 100.0;

const double COOLING_RATE_NORMAL = 0.95;
const double COOLING_RATE_FAST = 0.91;
const double COOLING_RATE_SLOW = 0.99;

const double REHEAT_FACTOR = 1.5; 
const double MAX_T_LIMIT = INITIAL_TEMPERATURE * 2.0; // Maksymalny sufit dla reheatingu
const int MAX_REHEAT_COUNT = 5; // Ile razy w ogóle pozwalamy na podgrzanie

const double MIN_TEMPERATURE = 0.2;
const double HIGH_TEMPERATURE = INITIAL_TEMPERATURE * 0.8;
const double MEDIUM_TEMPERATURE = INITIAL_TEMPERATURE * 0.6;
const double LOW_TEMPERATURE = INITIAL_TEMPERATURE * 0.4;



int evaluate_solution(std::vector<Task>& tasks);

std::vector<Task> generate_neighbor(std::vector<Task>& current_solution, double temperature);


#endif