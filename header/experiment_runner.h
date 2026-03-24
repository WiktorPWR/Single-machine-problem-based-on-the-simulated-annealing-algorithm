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
#include <numeric>

// ──────────────────────────────────────────────────────────────────────────────
//  Liczba powtórzeń każdego eksperymentu (100 runów, ten sam zbiór zadań,
//  różne seedy dla generatora losowości SA).
// ──────────────────────────────────────────────────────────────────────────────
static constexpr int NUM_RUNS = 100;

// Generuje zadania wg procentów typów zdefiniowanych w ExperimentParams.
// Jeśli pct_small=pct_medium=pct_large=0 → zachowanie losowe (oryginalne).
// seed jest STAŁY dla każdego eksperymentu – wszystkie 100 runów dostają ten sam
// zbiór zadań.
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
        std::uniform_int_distribution<int> dur_dist(1, MAX_DURATION);
        for (int i = 0; i < p.num_tasks; ++i)
            tasks.emplace_back(i, dur_dist(rng), pri_dist(rng),
                               due_dist(rng), rel_dist(rng));
        return tasks;
    }

    if (!budget) {
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

    // typed + budżet
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
//  Pojedynczy run SA – zwraca historię (iteration, cost, temperature)
//  sampled co 100 iteracji.
//
//  tasks       – STAŁY zbiór zadań (ten sam dla wszystkich runów)
//  cfg         – parametry eksperymentu
//  sa_seed     – seed wyłącznie dla generatora losowości SA (różny dla każdego runu)
// ──────────────────────────────────────────────────────────────────────────────
struct RunResult {
    std::vector<long long> iter_history;
    std::vector<double>    cost_history;   // double żeby umożliwić uśrednianie
    std::vector<double>    temp_history;
    int    best_cost;
    long long total_iterations;
    double T_final;
};

static RunResult run_single(const std::vector<Task>& tasks,
                             const ExperimentParams&  cfg,
                             unsigned                 sa_seed)
{
    std::mt19937 gen(sa_seed);
    std::uniform_real_distribution<double> dis(0.0, 1.0);

    std::vector<Task> best_solution    = tasks;
    std::vector<Task> current_solution = tasks;
    int best_cost    = evaluate_solution(best_solution);
    int current_cost = best_cost;

    RunResult res;
    double T     = cfg.initial_temperature;
    double T_min = cfg.min_temperature;

    int    reheat_counter    = 0;
    long long total_iterations  = 0;
    long long no_improve_streak = 0;

    const auto time_limit = std::chrono::seconds(cfg.time_limit_seconds);
    const auto start_time = std::chrono::steady_clock::now();

    double max_t_limit = cfg.initial_temperature * cfg.max_t_limit_multiplier;

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
                res.iter_history.push_back(total_iterations);
                res.cost_history.push_back(static_cast<double>(best_cost));
                res.temp_history.push_back(T);
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

    // ostatni punkt
    res.iter_history.push_back(total_iterations);
    res.cost_history.push_back(static_cast<double>(best_cost));
    res.temp_history.push_back(T);

    res.best_cost        = best_cost;
    res.total_iterations = total_iterations;
    res.T_final          = T;
    return res;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Uśrednianie RunResult-ów
//
//  Różne runy mogą mieć różną długość historii (różne warunki stopu).
//  Strategia: interpolacja liniowa – dla każdego punktu siatki wynikowej
//  wyznaczamy wartość z każdego runu metodą last-value-carry-forward
//  (wartość jest najlepszym kosztem do tej pory, więc jest nierosnąca).
//
//  Siatka: zbiór union wszystkich iter_history ze wszystkich runów,
//  ograniczony do max 2000 punktów (decymacja).
// ──────────────────────────────────────────────────────────────────────────────
struct AveragedResult {
    std::vector<long long> iter_grid;
    std::vector<double>    avg_cost;
    std::vector<double>    std_cost;
    std::vector<double>    avg_temp;
    int    avg_best_cost;    // średni best_cost po zakończeniu wszystkich runów
    double std_best_cost;
    long long avg_total_iter;
};

static AveragedResult average_runs(const std::vector<RunResult>& runs)
{
    // 1. Zbierz wszystkie punkty siatki (iter_history ze wszystkich runów)
    std::vector<long long> all_iters;
    for (auto& r : runs)
        for (auto v : r.iter_history)
            all_iters.push_back(v);

    std::sort(all_iters.begin(), all_iters.end());
    all_iters.erase(std::unique(all_iters.begin(), all_iters.end()), all_iters.end());

    // Decymacja do maks. 2000 punktów
    const size_t MAX_GRID = 2000;
    std::vector<long long> grid;
    if (all_iters.size() <= MAX_GRID) {
        grid = all_iters;
    } else {
        double step = static_cast<double>(all_iters.size() - 1) / (MAX_GRID - 1);
        for (size_t i = 0; i < MAX_GRID; ++i)
            grid.push_back(all_iters[static_cast<size_t>(std::round(i * step))]);
        // upewnij się że ostatni punkt jest włączony
        grid.back() = all_iters.back();
    }

    // 2. Dla każdego punktu siatki interpoluj wartości z każdego runu
    //    (last-value-carry-forward)
    size_t G = grid.size();
    std::vector<double> sum_cost(G, 0.0), sum_sq_cost(G, 0.0), sum_temp(G, 0.0);

    for (auto& r : runs) {
        for (size_t gi = 0; gi < G; ++gi) {
            long long query = grid[gi];
            // znajdź ostatni indeks w r.iter_history <= query
            auto it = std::upper_bound(r.iter_history.begin(),
                                        r.iter_history.end(), query);
            double cost, temp;
            if (it == r.iter_history.begin()) {
                // przed pierwszym punktem → użyj pierwszego punktu
                cost = r.cost_history.front();
                temp = r.temp_history.front();
            } else {
                --it;
                size_t idx = static_cast<size_t>(std::distance(r.iter_history.begin(), it));
                cost = r.cost_history[idx];
                temp = r.temp_history[idx];
            }
            sum_cost[gi]    += cost;
            sum_sq_cost[gi] += cost * cost;
            sum_temp[gi]    += temp;
        }
    }

    double n = static_cast<double>(runs.size());
    AveragedResult avg;
    avg.iter_grid.resize(G);
    avg.avg_cost.resize(G);
    avg.std_cost.resize(G);
    avg.avg_temp.resize(G);

    for (size_t gi = 0; gi < G; ++gi) {
        avg.iter_grid[gi] = grid[gi];
        avg.avg_cost[gi]  = sum_cost[gi]  / n;
        avg.avg_temp[gi]  = sum_temp[gi]  / n;
        double variance   = (sum_sq_cost[gi] / n) - (avg.avg_cost[gi] * avg.avg_cost[gi]);
        avg.std_cost[gi]  = std::sqrt(std::max(0.0, variance));
    }

    // 3. Statystyki końcowego best_cost
    double sum_bc = 0.0, sum_sq_bc = 0.0;
    long long sum_iter = 0;
    for (auto& r : runs) {
        sum_bc    += r.best_cost;
        sum_sq_bc += static_cast<double>(r.best_cost) * r.best_cost;
        sum_iter  += r.total_iterations;
    }
    avg.avg_best_cost  = static_cast<int>(std::round(sum_bc / n));
    double var_bc      = (sum_sq_bc / n) - (sum_bc / n) * (sum_bc / n);
    avg.std_best_cost  = std::sqrt(std::max(0.0, var_bc));
    avg.avg_total_iter = sum_iter / static_cast<long long>(runs.size());

    return avg;
}

// ──────────────────────────────────────────────────────────────────────────────
//  Główna funkcja uruchamiająca NUM_RUNS runów i zapisująca uśredniony CSV
//
//  Nazwa pliku CSV = bez zmian (cfg.experiment_name + ".csv")
//  Format CSV = rozszerzony o kolumny Std_Cost (odchylenie std.) dla każdego
//  punktu na krzywej.
// ──────────────────────────────────────────────────────────────────────────────
inline void run_experiment(const ExperimentParams& cfg,
                           const std::string& base_output_dir,
                           unsigned seed = 42)
{
    // ── Ustaw globalną macierz przezbrojenia ──
    if (cfg.use_custom_setup_matrix) {
        SETUP_MATRIX = cfg.setup_matrix;
    } else {
        reset_setup_matrix();
    }

    // ── Generuj JEDEN zestaw zadań (stały seed → ten sam zbiór dla wszystkich runów) ──
    const std::vector<Task> fixed_tasks = generate_tasks_typed(cfg, seed);

    if (cfg.initial_temperature <= 0.0 ||
        cfg.min_temperature     <= 0.0 ||
        cfg.min_temperature     >= cfg.initial_temperature) {
        std::cerr << "[WARN] Eksperyment " << cfg.experiment_name
                  << ": nieprawidłowa konfiguracja temperatury T="
                  << cfg.initial_temperature
                  << " T_min=" << cfg.min_temperature << " – pomijam.\n";
        return;
    }

    // ── Uruchom NUM_RUNS runów SA (każdy z innym seedem losowości SA) ──
    std::vector<RunResult> runs;
    runs.reserve(NUM_RUNS);

    std::cout << "  [" << cfg.category << "/" << cfg.experiment_name << "]"
              << "  uruchamiam " << NUM_RUNS << " runów...\n";

    for (int run_idx = 0; run_idx < NUM_RUNS; ++run_idx) {
        // seed SA = seed + 1337 + run_idx * 999983 (duże przesunięcia dla niezależności)
        unsigned sa_seed = seed + 1337u + static_cast<unsigned>(run_idx) * 999983u;
        runs.push_back(run_single(fixed_tasks, cfg, sa_seed));
    }

    // ── Uśrednij wyniki ──
    AveragedResult avg = average_runs(runs);

    // ── Wypisz podsumowanie ──
    std::cout << "  [" << cfg.category << "/" << cfg.experiment_name << "]"
              << "  avg_best_cost=" << avg.avg_best_cost
              << "  ±" << static_cast<int>(std::round(avg.std_best_cost))
              << "  avg_iter=" << avg.avg_total_iter
              << "  T_final(avg)=" << avg.avg_temp.back()
              << "\n";

    // ── Zapis CSV (ta sama nazwa pliku co wcześniej) ──
    std::string folder = base_output_dir + "/" + cfg.category;
    std::filesystem::create_directories(folder);
    std::string filepath = folder + "/" + cfg.experiment_name + ".csv";

    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "BLAD: nie mozna otworzyc " << filepath << "\n";
        return;
    }

    // Metadane
    file << "# experiment="     << cfg.experiment_name   << "\n";
    file << "# category="       << cfg.category          << "\n";
    file << "# num_runs="       << NUM_RUNS               << "\n";
    file << "# avg_best_cost="  << avg.avg_best_cost      << "\n";
    file << "# std_best_cost="  << avg.std_best_cost      << "\n";
    file << "# avg_total_iter=" << avg.avg_total_iter     << "\n";
    file << "# T_final_avg="    << avg.avg_temp.back()    << "\n";
    file << "# num_tasks="      << cfg.num_tasks          << "\n";
    file << "# stop_cond="      << cfg.stop_condition     << "\n";

    if (cfg.use_custom_setup_matrix) {
        file << "# setup_matrix=";
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                file << cfg.setup_matrix[r][c] << (r==2 && c==2 ? "" : ",");
        file << "\n";
    }

    // Nagłówek: uśredniona krzywa + odchylenie standardowe kosztu
    file << "Iteration,Avg_Cost,Std_Cost,Avg_Temperature\n";
    for (size_t i = 0; i < avg.iter_grid.size(); ++i) {
        file << avg.iter_grid[i] << ","
             << avg.avg_cost[i]  << ","
             << avg.std_cost[i]  << ","
             << avg.avg_temp[i]  << "\n";
    }
    file.close();
}

#endif // EXPERIMENT_RUNNER_H