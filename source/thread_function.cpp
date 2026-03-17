#include "thread_function.h"
#include <random>
#include <filesystem>
#include <string>  // To naprawi błąd 'to_string' i operator '+'
#include <fstream> // To pozwoli na zapis do pliku (ofstream)

void thread_simulated_annealing_function(std::vector<Task>& tasks,int thread_id) {
    
    // 1. Tworzymy unikalny generator dla tego konkretnego wątku
    // Używamy thread_id oraz czasu, żeby ziarno było na pewno inne
    std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count() + thread_id);
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    std::uniform_int_distribution<int> task_dis(0, tasks.size() - 1);


    std::vector<Task> best_solution = tasks;
    int best_cost = evaluate_solution(best_solution);

    // --- LOGOWANIE DANYCH ---
    std::vector<int> cost_history; 
    std::vector<double> temp_history; // Nowy wektor na temperaturę

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
                if (dis(gen) < p_accept) { 
                    best_solution = neighbor_solution;
                    best_cost = neighbor_cost;
                    accepted++;
                }
            }
            // LOGOWANIE KAŻDEJ ITERACJI WEWNĘTRZNEJ
            cost_history.push_back(best_cost);
            temp_history.push_back(T);
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

    // 1. Definiujemy ścieżkę do folderu
    std::string folder_path = "D:\\Pulpit\\PWR\\algorytmy optymalizacji\\data_output";

    // 2. (Opcjonalnie) Automatyczne tworzenie folderu, jeśli nie istnieje
    std::filesystem::create_directories(folder_path);

    // 3. Łączymy ścieżkę z nazwą pliku
    std::string filename = folder_path + "\\output_thread_" + std::to_string(thread_id) + ".csv";

    // 4. Zapis do pliku
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "Iteration,Cost,Temperature\n";
        for (size_t i = 0; i < cost_history.size(); ++i) {
            file << i << "," << cost_history[i] << "," << temp_history[i] << "\n";
        }
        file.close();
    } else {
        // Używamy locka, żeby cout się nie "rozjechał" przy błędzie
        std::lock_guard<std::mutex> lock(result_mutex);
        std::cerr << "BŁĄD: Nie można otworzyć pliku do zapisu: " << filename << std::endl;
    }

    // Koniec wylicznia wartosc zatem przechodzimy do zpaisu do mutexu
    std::lock_guard<std::mutex> lock(result_mutex);
    best_costs.push_back(best_cost);
    std::cout << "Wątek " << thread_id << " zakończył. Najlepszy koszt: " << best_cost << "\n";


}