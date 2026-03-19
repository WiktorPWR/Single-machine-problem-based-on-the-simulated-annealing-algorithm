#include "thread_function.h"
#include <random>
#include <filesystem>
#include <string>
#include <fstream>
#include <chrono>

void thread_simulated_annealing_function(std::vector<Task>& tasks, int thread_id) {

    std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count() + thread_id);
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    std::uniform_int_distribution<int> task_dis(0, tasks.size() - 1);

    std::vector<Task> best_solution = tasks;
    int best_cost = evaluate_solution(best_solution);

    std::vector<int> cost_history;
    std::vector<double> temp_history;

    double T = INITIAL_TEMPERATURE;
    double T_min = MIN_TEMPERATURE;

    int reheat_counter = 0;

    const long long MAX_TOTAL_ITERATIONS = 10'000'000LL;
    long long total_iterations = 0;

    const auto time_limit = std::chrono::seconds(60);
    const auto start_time = std::chrono::steady_clock::now();

    if (T <= 0.0 || T_min <= 0.0 || T_min >= T) {
        std::lock_guard<std::mutex> lock(result_mutex);
        std::cerr << "Wątek " << thread_id << ": Błędna konfiguracja temperatury!\n";
        return;
    }

    while (T > T_min) {

        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed >= time_limit) {
            std::lock_guard<std::mutex> lock(result_mutex);
            std::cout << "Wątek " << thread_id << ": Przekroczono limit czasu.\n";
            break;
        }

        if (total_iterations >= MAX_TOTAL_ITERATIONS) {
            std::lock_guard<std::mutex> lock(result_mutex);
            std::cout << "Wątek " << thread_id << ": Osiągnięto twardy limit iteracji.\n";
            break;
        }

        int accepted = 0;
        for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration) {
            total_iterations++;

            std::vector<Task> neighbor_solution = generate_neighbor(best_solution, T);
            int neighbor_cost = evaluate_solution(neighbor_solution);
            int delta = neighbor_cost - best_cost;

            if (delta < 0) {
                best_solution = neighbor_solution;
                best_cost = neighbor_cost;
                accepted++;
            } else {
                double exponent = -static_cast<double>(delta) / T;
                if (exponent > -700.0) {
                    double p_accept = std::exp(exponent);
                    if (dis(gen) < p_accept) {
                        best_solution = neighbor_solution;
                        best_cost = neighbor_cost;
                        accepted++;
                    }
                }
            }

            // Loguj co 100 iterację żeby nie zabić pamięci
            if (iteration % 100 == 0) {
                cost_history.push_back(best_cost);
                temp_history.push_back(T);
            }
        }

        double acceptance_rate = static_cast<double>(accepted) / MAX_ITERATIONS;

        if (acceptance_rate == 0.0) {
            if (reheat_counter < MAX_REHEAT_COUNT && T * REHEAT_FACTOR < MAX_T_LIMIT) {
                T *= REHEAT_FACTOR;
                reheat_counter++;
            } else {
                T *= COOLING_RATE_NORMAL;
            }
        } else if (acceptance_rate < 0.4) {
            T *= COOLING_RATE_SLOW;
        } else if (acceptance_rate > 0.6) {
            T *= COOLING_RATE_FAST;
        } else {
            T *= COOLING_RATE_NORMAL;
        }

        if (T <= 0.0 || !std::isfinite(T)) {
            std::lock_guard<std::mutex> lock(result_mutex);
            std::cerr << "Wątek " << thread_id << ": Temperatura stała się nieprawidłowa (" << T << ")!\n";
            break;
        }

        if (reheat_counter >= MAX_REHEAT_COUNT && T < T_min * 1.1) {
            break;
        }
    }

    // --- PRINT 1: Koniec liczenia ---
    {
        std::lock_guard<std::mutex> lock(result_mutex);
        std::cout << "Wątek " << thread_id << " zakończył liczenie."
                  << " Najlepszy koszt: " << best_cost
                  << " | Iteracje: " << total_iterations << "\n";
    }

    // --- ZAPIS DO PLIKU ---
    std::string folder_path = "D:\\Pulpit\\PWR\\algorytmy optymalizacji\\data_output";
    std::filesystem::create_directories(folder_path);
    std::string filename = folder_path + "\\output_thread_" + std::to_string(thread_id) + ".csv";

    // --- PRINT 2: Początek zapisu ---
    std::cout << "Wątek " << thread_id << " zapisuje " << cost_history.size() << " wierszy do CSV...\n";

    std::ofstream file(filename);
    if (file.is_open()) {
        file << "Iteration,Cost,Temperature\n";
        for (size_t i = 0; i < cost_history.size(); ++i) {
            file << i << "," << cost_history[i] << "," << temp_history[i] << "\n";
        }
        file.close();

        // --- PRINT 3: Koniec zapisu ---
        std::cout << "Wątek " << thread_id << " zakończył zapis CSV.\n";
    } else {
        std::lock_guard<std::mutex> lock(result_mutex);
        std::cerr << "BŁĄD: Nie można otworzyć pliku: " << filename << "\n";
    }

    std::lock_guard<std::mutex> lock(result_mutex);
    best_costs.push_back(best_cost);
}