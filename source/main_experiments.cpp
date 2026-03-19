// main_experiments.cpp
// Uruchamia wszystkie eksperymenty SA i zapisuje wyniki do CSV.
// Kompilacja (przykład):
//   g++ -std=c++17 -O2 -o run_experiments \
//       main_experiments.cpp task.cpp task_generator.cpp simulated_annealing.cpp \
//       -pthread
// Uruchomienie:
//   ./run_experiments [katalog_wyjsciowy]
//   (domyślnie: ./results)

#include <iostream>
#include <string>
#include <chrono>

#include <mutex>    // <- dodaj
#include <vector>   // <- dodaj

// Definicje wymagane przez thread_function.cpp
std::mutex result_mutex;      // <- dodaj
std::vector<int> best_costs;  // <- dodaj

#include "config.h"
#include "task.h"
#include "task_generator.h"
#include "simulated_annealing.h"
#include "experiment_config.h"
#include "experiment_runner.h"

int main(int argc, char* argv[]) {

    std::string output_dir = "D:\\Pulpit\\PWR\\algorytmy optymalizacji\\results";
    if (argc >= 2) output_dir = std::string(argv[1]);

    std::cout << "=== SA Experiment Suite ===\n";
    std::cout << "Wyniki beda zapisane do: " << output_dir << "\n\n";

    auto experiments = build_all_experiments();
    std::cout << "Liczba eksperymentow: " << experiments.size() << "\n\n";

    auto global_start = std::chrono::steady_clock::now();

    for (int i = 0; i < (int)experiments.size(); ++i) {
        const auto& exp = experiments[i];
        std::cout << "[" << (i+1) << "/" << experiments.size() << "] "
                  << exp.category << "/" << exp.experiment_name << " ...\n";

        // Każdy eksperyment ma unikalny seed (deterministyczny ale różny)
        unsigned seed = 42 + (unsigned)i * 137;
        run_experiment(exp, output_dir, seed);
    }

    auto global_end = std::chrono::steady_clock::now();
    double total_sec = std::chrono::duration<double>(global_end - global_start).count();

    std::cout << "\n=== Wszystkie eksperymenty zakonczone ===\n";
    std::cout << "Czas calkowity: " << (int)(total_sec / 60) << "m "
              << (int)(total_sec) % 60 << "s\n";
    std::cout << "Wyniki w: " << output_dir << "\n";

    return 0;
}
