#ifndef EXPERIMENT_RUNNER_H
#define EXPERIMENT_RUNNER_H

#include "experiment_config.h"
#include "config.h"
#include "task.h"
#include "task_generator.h"
#include "simulated_annealing.h"

#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <random>
#include <cmath>
#include <algorithm>

// Generuje zadania wg procentów typów zdefiniowanych w ExperimentParams.
// Jeśli pct_small=pct_medium=pct_large=0 → zachowanie losowe (oryginalne).
static std::vector<Task> generate_tasks_typed(const ExperimentParams& p, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> pri_dist(1, MAX_PRIORITY);
    std::uniform_int_distribution<int> due_dist(1, MAX_DUE_TIME);
    std::uniform_int_distribution<int> rel_dist(1, MAX_RELEASE_TIME);

    bool typed  = (p.pct_small + p.pct_medium + p.pct_large) > 0.001;
    bool budget = (p.total_duration_budget > 0);

    // Progi duration (zgodne z task.cpp)
    const int MEDIUM_THRESH = (int)(MAX_DURATION * 0.5);  // >= 50
    const int LARGE_THRESH  = (int)(MAX_DURATION * 0.8);  // >= 80

    int n_small  = typed ? (int)std::round(p.pct_small  * p.num_tasks) : 0;
    int n_medium = typed ? (int)std::round(p.pct_medium * p.num_tasks) : 0;
    int n_large  = typed ? p.num_tasks - n_small - n_medium             : 0;

    std::vector<Task> tasks;
    tasks.reserve(p.num_tasks);

    if (!typed) {
        // ---- oryginalne losowe (bez budżetu) ----
        std::uniform_int_distribution<int> dur_dist(1, MAX_DURATION);
        for (int i = 0; i < p.num_tasks; ++i)
            tasks.emplace_back(i, dur_dist(rng), pri_dist(rng),
                               due_dist(rng), rel_dist(rng));
        return tasks;
    }

    if (!budget) {
        // ---- typed bez budżetu ----
        std::uniform_int_distribution<int> dur_small (1, MEDIUM_THRESH - 1);
        std::uniform_int_distribution<int> dur_medium(MEDIUM_THRESH, LARGE_THRESH - 1);
        std::uniform_int_distribution<int> dur_large (LARGE_THRESH,  MAX_DURATION);
        int id = 0;
        for (int i = 0; i < n_small;  ++i, ++id)
            tasks.emplace_back(id, dur_small(rng),  pri_dist(rng), due_dist(rng), rel_dist(rng));
        for (int i = 0; i < n_medium; ++i, ++id)
            tasks.emplace_back(id, dur_medium(rng), pri_dist(rng), due_dist(rng), rel_dist(rng));
        for (int i = 0; i < n_large;  ++i, ++id)
            tasks.emplace_back(id, dur_large(rng),  pri_dist(rng), due_dist(rng), rel_dist(rng));
        std::shuffle(tasks.begin(), tasks.end(), rng);
        return tasks;
    }

    // ──────────────────────────────────────────────────────────
    //  typed + budżet: suma duration == total_duration_budget
    // ──────────────────────────────────────────────────────────
    int budget_small  = (int)std::round(p.pct_small  * p.total_duration_budget);
    int budget_medium = (int)std::round(p.pct_medium * p.total_duration_budget);
    int budget_large  = p.total_duration_budget - budget_small - budget_medium;

    auto gen_group = [&](int n, int typ_budget, int dur_min, int dur_max,
                         int start_id) -> std::vector<Task>
    {
        std::vector<Task> group;
        if (n == 0) return group;

        std::uniform_int_distribution<int> d(dur_min, dur_max);
        std::vector<int> durations(n);
        int raw_sum = 0;
        for (int i = 0; i < n; ++i) { durations[i] = d(rng); raw_sum += durations[i]; }

        int assigned = 0;
        for (int i = 0; i < n - 1; ++i) {
            int scaled = (int)std::round((double)durations[i] / raw_sum * typ_budget);
            scaled = std::max(dur_min, std::min(dur_max, scaled));
            durations[i] = scaled;
            assigned += scaled;
        }
        int last = std::max(dur_min, std::min(dur_max, typ_budget - assigned));
        durations[n - 1] = last;

        for (int i = 0; i < n; ++i)
            group.emplace_back(start_id + i, durations[i],
                               pri_dist(rng), due_dist(rng), rel_dist(rng));
        return group;
    };

    auto g_small  = gen_group(n_small,  budget_small,  1,             MEDIUM_THRESH - 1, 0);
    auto g_medium = gen_group(n_medium, budget_medium, MEDIUM_THRESH, LARGE_THRESH  - 1, n_small);
    auto g_large  = gen_group(n_large,  budget_large,  LARGE_THRESH,  MAX_DURATION,      n_small + n_medium);

    for (auto& t : g_small)  tasks.push_back(t);
    for (auto& t : g_medium) tasks.push_back(t);
    for (auto& t : g_large)  tasks.push_back(t);

    std::shuffle(tasks.begin(), tasks.end(), rng);
    return tasks;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Główna funkcja uruchamiająca jeden eksperyment i zapisująca CSV
// ──────────────────────────────────────────────────────────────────────────────
inline void run_experiment(const ExperimentParams& cfg,
                           const std::string& base_output_dir,
                           unsigned seed = 42)
{
    // ── Ustaw globalną macierz przezbrojenia PRZED generowaniem zadań ──
    // Kategoria 3_setup_times: używa niestandardowej macierzy z cfg.setup_matrix
    // Wszystkie inne kategorie: macierz domyślna z config.h (per-typ, bez różnicowania celu)
    if (cfg.use_custom_setup_matrix) {
        SETUP_MATRIX = cfg.setup_matrix;
    } else {
        reset_setup_matrix();
    }

    // ---- generuj zadania ----
    std::vector<Task> tasks = generate_tasks_typed(cfg, seed);

    // ---- inicjalizacja SA ----
    std::mt19937 gen(seed + 1337);
    std::uniform_real_distribution<double> dis(0.0, 1.0);

    std::vector<Task> best_solution    = tasks;
    std::vector<Task> current_solution = tasks;
    int best_cost    = evaluate_solution(best_solution);
    int current_cost = best_cost;

    std::vector<long long> iter_history;
    std::vector<int>       cost_history;
    std::vector<double>    temp_history;

    double T     = cfg.initial_temperature;
    double T_min = cfg.min_temperature;

    if (T <= 0.0 || T_min <= 0.0 || T_min >= T) {
        std::cerr << "[WARN] Eksperyment " << cfg.experiment_name
                  << ": nieprawidłowa konfiguracja temperatury T=" << T
                  << " T_min=" << T_min << " – pomijam.\n";
        return;
    }

    int    reheat_counter    = 0;
    long long total_iterations  = 0;
    long long no_improve_streak = 0;

    const auto time_limit = std::chrono::seconds(cfg.time_limit_seconds);
    const auto start_time = std::chrono::steady_clock::now();

    double max_t_limit = cfg.initial_temperature * cfg.max_t_limit_multiplier;

    // ---- pętla główna SA ----
    bool stop = false;
    while (!stop && T > T_min) {

        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed >= time_limit) break;

        if (cfg.stop_condition == "iterations" &&
            total_iterations >= cfg.max_total_iterations) break;

        if (total_iterations >= cfg.max_total_iterations) break;

        int accepted = 0;
        for (int it = 0; it < cfg.max_iterations_per_temp && !stop; ++it) {
            ++total_iterations;

            std::vector<Task> neighbor = generate_neighbor(current_solution, T);
            int neighbor_cost = evaluate_solution(neighbor);
            int delta = neighbor_cost - current_cost;

            if (delta < 0) {
                current_solution = neighbor;
                current_cost     = neighbor_cost;
                accepted++;
                no_improve_streak = 0;
                if (current_cost < best_cost) {
                    best_solution = current_solution;
                    best_cost     = current_cost;
                }
            } else {
                double exponent = -static_cast<double>(delta) / T;
                if (exponent > -700.0 && dis(gen) < std::exp(exponent)) {
                    current_solution = neighbor;
                    current_cost     = neighbor_cost;
                    accepted++;
                } else {
                    ++no_improve_streak;
                }
            }

            if (cfg.stop_condition == "no_improve" &&
                no_improve_streak >= cfg.no_improve_limit) {
                stop = true;
            }

            if (total_iterations % 100 == 0) {
                iter_history.push_back(total_iterations);
                cost_history.push_back(best_cost);
                temp_history.push_back(T);
            }
        }

        // ---- aktualizacja temperatury ----
        double acceptance_rate = static_cast<double>(accepted) / cfg.max_iterations_per_temp;

        if (cfg.adaptive_cooling) {
            if (acceptance_rate == 0.0) {
                if (cfg.enable_reheat &&
                    reheat_counter < cfg.max_reheat_count &&
                    T * cfg.reheat_factor < max_t_limit) {
                    T *= cfg.reheat_factor;
                    reheat_counter++;
                } else {
                    T *= cfg.cooling_rate_normal;
                }
            } else if (acceptance_rate < cfg.accept_low_thresh) {
                T *= cfg.cooling_rate_slow;
            } else if (acceptance_rate > cfg.accept_high_thresh) {
                T *= cfg.cooling_rate_fast;
            } else {
                T *= cfg.cooling_rate_normal;
            }
        } else {
            if (cfg.enable_reheat &&
                acceptance_rate == 0.0 &&
                reheat_counter < cfg.max_reheat_count &&
                T * cfg.reheat_factor < max_t_limit) {
                T *= cfg.reheat_factor;
                reheat_counter++;
            } else {
                T *= cfg.cooling_rate;
            }
        }

        if (T <= 0.0 || !std::isfinite(T)) break;

        if (cfg.enable_reheat &&
            reheat_counter >= cfg.max_reheat_count &&
            T < T_min * 1.1) break;
    }

    // ---- zapisz ostatni punkt ----
    iter_history.push_back(total_iterations);
    cost_history.push_back(best_cost);
    temp_history.push_back(T);

    // ---- wypisz podsumowanie ----
    std::cout << "  [" << cfg.category << "/" << cfg.experiment_name << "]"
              << "  koszt=" << best_cost
              << "  iter=" << total_iterations
              << "  T_final=" << T
              << "\n";

    // ---- zapis CSV ----
    std::string folder = base_output_dir + "/" + cfg.category;
    std::filesystem::create_directories(folder);
    std::string filepath = folder + "/" + cfg.experiment_name + ".csv";

    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "BLAD: nie mozna otworzyc " << filepath << "\n";
        return;
    }

    // Metadane jako komentarz w nagłówku
    file << "# experiment=" << cfg.experiment_name << "\n";
    file << "# category="   << cfg.category        << "\n";
    file << "# best_cost="  << best_cost            << "\n";
    file << "# total_iter=" << total_iterations     << "\n";
    file << "# T_final="    << T                    << "\n";
    file << "# num_tasks="  << cfg.num_tasks        << "\n";
    file << "# stop_cond="  << cfg.stop_condition   << "\n";

    // Zapisz macierz przezbrojenia użytą w tym eksperymencie
    if (cfg.use_custom_setup_matrix) {
        file << "# setup_matrix=";
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                file << cfg.setup_matrix[r][c] << (r==2 && c==2 ? "" : ",");
        file << "\n";
    }

    file << "Iteration,Cost,Temperature\n";
    for (size_t i = 0; i < cost_history.size(); ++i) {
        file << iter_history[i] << ","
             << cost_history[i] << ","
             << temp_history[i] << "\n";
    }
    file.close();
}

#endif // EXPERIMENT_RUNNER_H