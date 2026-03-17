#include <iostream>
#include <fstream> // Potrzebne do zapisu pliku
#include <vector>
#include <cmath>
#include "task.h"
#include "task_generator.h"
#include "simulated_annealing.h"

const int NUM_TASKS = 10;
const int MAX_DURATION = 100;
const int MAX_PRIORITY = 10;
const int MAX_DUE_TIME = 100;
const int MAX_RELEASE_TIME = 100;


int main() {
    TaskGenerator generator;
    std::vector<Task> tasks = generator.generate_tasks(NUM_TASKS, MAX_DURATION, MAX_PRIORITY, MAX_DUE_TIME, MAX_RELEASE_TIME);

    std::vector<Task> best_solution = tasks;
    int best_cost = evaluate_solution(best_solution);

    // --- LOGOWANIE DANYCH ---
    std::vector<int> cost_history; 
    cost_history.push_back(best_cost);
    // ------------------------

    double T = INITIAL_TEMPERATURE; 
    double T_min = MIN_TEMPERATURE; 

    while (T > T_min) {
        for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration) {
            std::vector<Task> neighbor_solution = generate_neighbor(best_solution);
            int neighbor_cost = evaluate_solution(neighbor_solution);

            int delta = neighbor_cost - best_cost;

            if (delta < 0) {
                best_solution = neighbor_solution;
                best_cost = neighbor_cost;
            } else {
                double p_accept = exp(-delta / T);
                if (((double)rand() / RAND_MAX) < p_accept) {
                    best_solution = neighbor_solution;
                    best_cost = neighbor_cost;
                }
            }
            // Zapisujemy koszt po każdej iteracji wewnętrznej
            cost_history.push_back(best_cost);
        }
        T *= COOLING_RATE;
    }

    // --- ZAPIS DO PLIKU CSV ---
    std::ofstream file("D:\\Pulpit\\PWR\\algorytmy optymalizacji\\output_data.csv");
    file << "Iteration,Cost\n";
    for (size_t i = 0; i < cost_history.size(); ++i) {
        file << i << "," << cost_history[i] << "\n";
    }
    file.close();
    std::cout << "Dane zapisane do output_data.csv. Mozesz teraz wygenerowac wykres." << std::endl;

    return 0;
}