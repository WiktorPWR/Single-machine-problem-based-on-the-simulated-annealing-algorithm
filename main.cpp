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
    std::vector<double> temp_history; // Nowy wektor na temperaturę
    
    cost_history.push_back(best_cost);
    temp_history.push_back(INITIAL_TEMPERATURE);
    // ------------------------

    double T = INITIAL_TEMPERATURE; 
    double T_min = MIN_TEMPERATURE; 

    int reheat_counter = 0; // Licznik użyć podgrzewania

    while (T > T_min) {
        int accepted = 0;
        for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration) {

            std::vector<Task> neighbor_solution = generate_neighbor(best_solution, T);
            int neighbor_cost = evaluate_solution(neighbor_solution);

            int delta = neighbor_cost - best_cost;

            if (delta < 0) {
                best_solution = neighbor_solution;
                best_cost = neighbor_cost;

                accepted++;

            } else {
                double p_accept = exp(-delta / T);
                if (((double)rand() / RAND_MAX) < p_accept) {
                    best_solution = neighbor_solution;
                    best_cost = neighbor_cost;
                    accepted++;
                }

            }

            
            // Zapisujemy oba parametry
            cost_history.push_back(best_cost);
            temp_history.push_back(T);
        }

        double acceptance_rate = (double)accepted / MAX_ITERATIONS;

        if (acceptance_rate == 0) {
            // Reheating z podwójnym zabezpieczeniem
            if (reheat_counter < MAX_REHEAT_COUNT && T * REHEAT_FACTOR < MAX_T_LIMIT) {
                T *= REHEAT_FACTOR;
                reheat_counter++;
                std::cout << "Reheating! Proba: " << reheat_counter << " Nowa Temp: " << T << std::endl;
            } else {
                // Jeśli wykorzystaliśmy limity, wymuszamy normalne chłodzenie, 
                // żeby algorytm kiedyś się skończył
                T *= COOLING_RATE_NORMAL; 
            }
        } 
        else if (acceptance_rate < 0.4) {
            T *= COOLING_RATE_SLOW;
        } 
        else if (acceptance_rate > 0.6) {
            T *= COOLING_RATE_FAST;
        } 
        else {
            T *= COOLING_RATE_NORMAL;
        }

        // Opcjonalnie: Jeśli T spadnie ekstremalnie nisko po wielu próbach, a wciąż podgrzewamy
        if (reheat_counter >= MAX_REHEAT_COUNT && T < T_min * 1.1) {
             break; // Wyjście awaryjne
        }
    }

    // --- ZAPIS DO PLIKU CSV ---
    std::ofstream file("D:\\Pulpit\\PWR\\algorytmy optymalizacji\\output_data.csv");
    file << "Iteration,Cost,Temperature\n"; // Nagłówek z 3 kolumnami
    for (size_t i = 0; i < cost_history.size(); ++i) {
        file << i << "," << cost_history[i] << "," << temp_history[i] << "\n";
    }
    file.close();
    
    std::cout << "Dane zapisane. Mozesz teraz wygenerowac dwa wykresy." << std::endl;
    return 0;
}