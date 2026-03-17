#include "thread_function.h"

void thread_simulated_annealing_function(std::vector<Task>& tasks,int thread_id) {
    
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
        }

        double acceptance_rate = (double)accepted / MAX_ITERATIONS;

        if (acceptance_rate == 0) {
            // Reheating z podwójnym zabezpieczeniem
            if (reheat_counter < MAX_REHEAT_COUNT && T * REHEAT_FACTOR < MAX_T_LIMIT) {
                T *= REHEAT_FACTOR;
                reheat_counter++;
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

    // Koniec wylicznia wartosc zatem przechodzimy do zpaisu do mutexu
    std::lock_guard<std::mutex> lock(result_mutex);
    best_costs.push_back(best_cost);
    std::cout << "Wątek " << thread_id << " zakończył. Najlepszy koszt: " << best_cost << "\n";


}