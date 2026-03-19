#ifndef EXPERIMENT_CONFIG_H
#define EXPERIMENT_CONFIG_H

#include <string>
#include <vector>

// =====================================================
//  Struktura opisujaca jeden eksperyment
// =====================================================
struct ExperimentParams {
    std::string experiment_name;   // nazwa folderu/pliku CSV
    std::string category;          // kategoria (podfolder)

    // --- SA parametry ---
    double initial_temperature;
    double min_temperature;
    double cooling_rate;           // używany gdy adaptive=false
    bool   adaptive_cooling;       // czy używać adaptive (acceptance-rate based)
    double cooling_rate_fast;
    double cooling_rate_slow;
    double cooling_rate_normal;

    // --- Reheat ---
    bool   enable_reheat;
    double reheat_factor;
    double max_t_limit_multiplier; // MAX_T_LIMIT = initial_temperature * multiplier
    int    max_reheat_count;

    // --- Iteracje ---
    int    max_iterations_per_temp; // iteracje na jedną temperaturę
    long long max_total_iterations;
    int    time_limit_seconds;

    // --- Warunek stopu ---
    // "temperature"  – stop gdy T < T_min
    // "no_improve"   – stop gdy N iteracji bez poprawy
    // "iterations"   – stop gdy osiągnięto max_total_iterations
    std::string stop_condition;
    long long   no_improve_limit;  // używane gdy stop_condition == "no_improve"

    // --- Zadania ---
    int num_tasks;
    double pct_small;   // 0..1
    double pct_medium;
    double pct_large;
    // pct_small + pct_medium + pct_large powinno == 1.0
    // Jeśli wszystkie == 0 → losowe (oryginalne zachowanie)

    // --- Acceptance rate progi (adaptive) ---
    double accept_high_thresh;  // > tego → fast cooling
    double accept_low_thresh;   // < tego → slow cooling
};

// =====================================================
//  Pomocnik: wypełnienie sensownymi domyślami
// =====================================================
inline ExperimentParams make_default() {
    ExperimentParams p;
    p.experiment_name           = "default";
    p.category                  = "misc";
    p.initial_temperature       = 10.0;
    p.min_temperature           = 1.0;
    p.cooling_rate              = 0.95;
    p.adaptive_cooling          = true;
    p.cooling_rate_fast         = 0.91;
    p.cooling_rate_slow         = 0.99;
    p.cooling_rate_normal       = 0.95;
    p.enable_reheat             = true;
    p.reheat_factor             = 1.5;
    p.max_t_limit_multiplier    = 2.0;
    p.max_reheat_count          = 5;
    p.max_iterations_per_temp   = 100;
    p.max_total_iterations      = 10'000'000LL;
    p.time_limit_seconds        = 60;
    p.stop_condition            = "temperature";
    p.no_improve_limit          = 500'000LL;
    p.num_tasks                 = 100;
    p.pct_small                 = 0.0;
    p.pct_medium                = 0.0;
    p.pct_large                 = 0.0;
    p.accept_high_thresh        = 0.6;
    p.accept_low_thresh         = 0.4;
    return p;
}

// =====================================================
//  Budowanie listy wszystkich eksperymentów
// =====================================================
inline std::vector<ExperimentParams> build_all_experiments() {
    std::vector<ExperimentParams> all;

    // ─────────────────────────────────────────────
    // 1. COOLING RATE
    // ─────────────────────────────────────────────
    {
        std::vector<double> rates = {0.85, 0.91, 0.95, 0.99};
        for (double r : rates) {
            auto p = make_default();
            p.category         = "1_cooling_rate";
            p.adaptive_cooling = false;   // stały cooling rate
            p.cooling_rate     = r;
            p.enable_reheat    = false;
            p.experiment_name  = "cooling_" + std::to_string((int)(r * 100));
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 2. ACCEPTANCE RATE THRESHOLDS (adaptive)
    // ─────────────────────────────────────────────
    {
        // (low_thresh, high_thresh)
        std::vector<std::pair<double,double>> thresholds = {
            {0.2, 0.5},
            {0.3, 0.6},
            {0.4, 0.7},
            {0.5, 0.8}
        };
        for (auto& [lo, hi] : thresholds) {
            auto p = make_default();
            p.category              = "2_acceptance_rate";
            p.adaptive_cooling      = true;
            p.accept_low_thresh     = lo;
            p.accept_high_thresh    = hi;
            p.experiment_name       = "accept_lo" + std::to_string((int)(lo*10))
                                    + "_hi" + std::to_string((int)(hi*10));
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 3. CZASY PRZEZBROJENIA – przez % zadań dużych
    //    (duże zadania → większy setup_time=20)
    // ─────────────────────────────────────────────
    {
        // różne miksy: (small%, medium%, large%)
        std::vector<std::tuple<double,double,double>> mixes = {
            {1.0, 0.0, 0.0},   // same małe  → mały setup
            {0.5, 0.5, 0.0},   // pół-pół
            {0.33, 0.33, 0.34},// równy miks
            {0.0, 0.0, 1.0},   // same duże  → duży setup
        };
        std::vector<std::string> names = {"all_small","half_medium","equal_mix","all_large"};
        for (int i = 0; i < (int)mixes.size(); ++i) {
            auto p = make_default();
            p.category    = "3_setup_times";
            auto& [s,m,l] = mixes[i];
            p.pct_small   = s;  p.pct_medium = m;  p.pct_large = l;
            p.experiment_name = "setup_" + names[i];
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 4. LICZBA ITERACJI na temperaturę
    // ─────────────────────────────────────────────
    {
        std::vector<int> iters = {50, 100, 500, 1000};
        for (int it : iters) {
            auto p = make_default();
            p.category                 = "4_iterations";
            p.max_iterations_per_temp  = it;
            p.experiment_name          = "iter_" + std::to_string(it);
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 5. REHEATING
    // ─────────────────────────────────────────────
    {
        // bez reheat
        {
            auto p = make_default();
            p.category        = "5_reheating";
            p.enable_reheat   = false;
            p.experiment_name = "reheat_none";
            all.push_back(p);
        }
        // różne max_reheat_count
        for (int rc : {2, 5, 10}) {
            auto p = make_default();
            p.category          = "5_reheating";
            p.enable_reheat     = true;
            p.max_reheat_count  = rc;
            p.experiment_name   = "reheat_count_" + std::to_string(rc);
            all.push_back(p);
        }
        // różny reheat_factor
        for (double rf : {1.2, 1.5, 2.0}) {
            auto p = make_default();
            p.category          = "5_reheating";
            p.enable_reheat     = true;
            p.reheat_factor     = rf;
            p.experiment_name   = "reheat_factor_" + std::to_string((int)(rf*10));
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 6. TEMPERATURA STARTOWA
    // ─────────────────────────────────────────────
    {
        std::vector<double> temps = {1.0, 5.0, 10.0, 50.0, 100.0};
        for (double t : temps) {
            auto p = make_default();
            p.category              = "6_start_temperature";
            p.initial_temperature   = t;
            p.max_t_limit_multiplier= 2.0;
            // min_temperature: 10% initial
            p.min_temperature       = t * 0.1;
            if (p.min_temperature < 0.01) p.min_temperature = 0.01;
            p.experiment_name       = "start_T_" + std::to_string((int)t);
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 7. TASK TYPE MIX (percentowy)
    // ─────────────────────────────────────────────
    {
        std::vector<std::tuple<double,double,double>> mixes = {
            {1.0, 0.0, 0.0},
            {0.7, 0.2, 0.1},
            {0.5, 0.3, 0.2},
            {0.2, 0.5, 0.3},
            {0.05, 0.15, 0.80},
            {0.0, 0.0, 1.0},
        };
        std::vector<std::string> names = {
            "s100_m0_l0",
            "s70_m20_l10",
            "s50_m30_l20",
            "s20_m50_l30",
            "s5_m15_l80",
            "s0_m0_l100"
        };
        for (int i = 0; i < (int)mixes.size(); ++i) {
            auto p = make_default();
            p.category    = "7_task_type";
            auto& [s,m,l] = mixes[i];
            p.pct_small   = s;  p.pct_medium = m;  p.pct_large = l;
            p.experiment_name = "tasktype_" + names[i];
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 8. WARUNEK STOPU
    // ─────────────────────────────────────────────
    {
        // a) klasyczny – stop na temperaturze
        {
            auto p = make_default();
            p.category        = "8_stop_condition";
            p.stop_condition  = "temperature";
            p.experiment_name = "stop_temperature";
            all.push_back(p);
        }
        // b) brak poprawy przez N iteracji
        for (long long ni : {10'000LL, 100'000LL, 500'000LL}) {
            auto p = make_default();
            p.category         = "8_stop_condition";
            p.stop_condition   = "no_improve";
            p.no_improve_limit = ni;
            p.experiment_name  = "stop_no_improve_" + std::to_string(ni);
            all.push_back(p);
        }
        // c) twardy limit iteracji
        for (long long mi : {100'000LL, 500'000LL, 2'000'000LL}) {
            auto p = make_default();
            p.category              = "8_stop_condition";
            p.stop_condition        = "iterations";
            p.max_total_iterations  = mi;
            p.experiment_name       = "stop_iter_limit_" + std::to_string(mi);
            all.push_back(p);
        }
    }

    return all;
}

#endif // EXPERIMENT_CONFIG_H
