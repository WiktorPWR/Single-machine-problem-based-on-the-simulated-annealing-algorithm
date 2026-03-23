#ifndef EXPERIMENT_CONFIG_H
#define EXPERIMENT_CONFIG_H

#include <string>
#include <vector>
#include <array>

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

    // --- Budżet czasowy zadań ---
    // Jeśli > 0: suma duration wszystkich zadań będzie równa total_duration_budget
    // Jeśli == 0: duration losowane niezależnie (oryginalne zachowanie)
    int total_duration_budget;

    // --- Acceptance rate progi (adaptive) ---
    double accept_high_thresh;  // > tego → fast cooling
    double accept_low_thresh;   // < tego → slow cooling

    // --- Macierz czasów przezbrojenia ---
    // Indeksy: 0=SMALL, 1=MEDIUM, 2=LARGE
    // setup_matrix[from][to] = czas przezbrojenia
    // Używana tylko w kategorii "3_setup_times".
    // Dla pozostałych kategorii: wszystkie wartości == 0.0
    // (wtedy experiment_runner ustawi macierz domyślną z config.h)
    std::array<std::array<double, 3>, 3> setup_matrix;

    // Czy ten eksperyment używa niestandardowej macierzy przezbrojenia
    bool use_custom_setup_matrix;
};

// =====================================================
//  Pomocnik: wypełnienie sensownymi domyślami
// =====================================================
inline ExperimentParams make_default() {
    ExperimentParams p;
    p.experiment_name           = "default";
    p.category                  = "misc";
    p.initial_temperature       = 100.0;
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
    p.total_duration_budget     = 0;
    p.accept_high_thresh        = 0.6;
    p.accept_low_thresh         = 0.4;
    p.use_custom_setup_matrix   = false;
    for (auto& row : p.setup_matrix) row.fill(0.0);
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
            p.adaptive_cooling = false;
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
        std::vector<std::pair<double,double>> thresholds = {
            {0.2, 0.5},
            {0.3, 0.6},
            {0.4, 0.7},
            {0.5, 0.8}
        };
        for (auto& [lo, hi] : thresholds) {
            auto p = make_default();
            p.category           = "2_acceptance_rate";
            p.adaptive_cooling   = true;
            p.accept_low_thresh  = lo;
            p.accept_high_thresh = hi;
            p.experiment_name    = "accept_lo" + std::to_string((int)(lo*10))
                                 + "_hi" + std::to_string((int)(hi*10));
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 3. CZASY PRZEZBROJENIA – macierz setup_matrix[from][to]
    //
    // Indeksy: 0=SMALL, 1=MEDIUM, 2=LARGE
    // experiment_runner przed uruchomieniem każdego eksperymentu
    // z tej kategorii wpisze macierz do globalnego SETUP_MATRIX.
    //
    // Warianty:
    //  A) uniform_low     – wszystkie przejścia jednakowo tanie (2)
    //  B) uniform_high    – wszystkie przejścia jednakowo drogie (20)
    //  C) symmetric       – koszt rośnie z różnicą rozmiaru zadań
    //  D) upgrade_costly  – przejście do większego zadania drogie
    //  E) downgrade_costly– przejście do mniejszego zadania drogie
    //  F) diagonal_free   – ten sam typ = 0, zmiana typu = 15
    //  G) realistic       – wartości zbliżone do przemysłowych
    // ─────────────────────────────────────────────
    {
        struct SetupVariant {
            std::string name;
            // from\to    S      M      L
            double ss, sm, sl;   // from SMALL
            double ms, mm, ml;   // from MEDIUM
            double ls, lm, ll;   // from LARGE
        };

        std::vector<SetupVariant> variants = {
            // A) Wszystkie przejścia jednakowo tanie
            { "uniform_low",
               2,  2,  2,
               2,  2,  2,
               2,  2,  2 },

            // B) Wszystkie przejścia jednakowo drogie
            { "uniform_high",
              20, 20, 20,
              20, 20, 20,
              20, 20, 20 },

            // C) Symetryczna – koszt proporcjonalny do różnicy rozmiaru
            //    S↔S=1, S↔M=5, S↔L=15, M↔M=1, M↔L=8, L↔L=1
            { "symmetric",
               1,  5, 15,
               5,  1,  8,
              15,  8,  1 },

            // D) Upgrade kosztowny – przejście do większego = drogie
            { "upgrade_costly",
               1, 10, 20,
               3,  1, 15,
               2,  4,  1 },

            // E) Downgrade kosztowny – przejście do mniejszego = drogie
            { "downgrade_costly",
               1,  3,  2,
              10,  1,  4,
              20, 15,  1 },

            // F) Diagonal free – zmiana typu zawsze droga, ten sam typ = 0
            { "diagonal_free",
               0, 15, 15,
              15,  0, 15,
              15, 15,  0 },

            // G) Realistyczne (asymetryczne)
            { "realistic",
               2,  6, 18,
               5,  2, 12,
              16, 10,  3 },
        };

        for (auto& v : variants) {
            auto p = make_default();
            p.category                = "3_setup_times";
            p.pct_small               = 0.33;
            p.pct_medium              = 0.34;
            p.pct_large               = 0.33;
            p.use_custom_setup_matrix = true;
            p.setup_matrix[0] = { v.ss, v.sm, v.sl };
            p.setup_matrix[1] = { v.ms, v.mm, v.ml };
            p.setup_matrix[2] = { v.ls, v.lm, v.ll };
            p.experiment_name = "setup_" + v.name;
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
            p.category                = "4_iterations";
            p.max_iterations_per_temp = it;
            p.experiment_name         = "iter_" + std::to_string(it);
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 5. REHEATING
    // ─────────────────────────────────────────────
    {
        {
            auto p = make_default();
            p.category        = "5_reheating";
            p.enable_reheat   = false;
            p.experiment_name = "reheat_none";
            all.push_back(p);
        }
        for (int rc : {2, 5, 10}) {
            auto p = make_default();
            p.category         = "5_reheating";
            p.enable_reheat    = true;
            p.max_reheat_count = rc;
            p.experiment_name  = "reheat_count_" + std::to_string(rc);
            all.push_back(p);
        }
        for (double rf : {1.2, 1.5, 2.0}) {
            auto p = make_default();
            p.category        = "5_reheating";
            p.enable_reheat   = true;
            p.reheat_factor   = rf;
            p.experiment_name = "reheat_factor_" + std::to_string((int)(rf*10));
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
            p.category               = "6_start_temperature";
            p.initial_temperature    = t;
            p.max_t_limit_multiplier = 2.0;
            p.min_temperature        = t * 0.1;
            if (p.min_temperature < 0.01) p.min_temperature = 0.01;
            p.experiment_name        = "start_T_" + std::to_string((int)t);
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 7. TASK TYPE MIX (percentowy, stały budżet)
    // ─────────────────────────────────────────────
    {
        const int TOTAL_BUDGET = 10000;

        std::vector<std::tuple<double,double,double>> mixes = {
            {1.00, 0.00, 0.00},
            {0.80, 0.20, 0.00},
            {0.70, 0.20, 0.10},
            {0.60, 0.30, 0.10},
            {0.50, 0.30, 0.20},
            {0.33, 0.34, 0.33},
            {0.20, 0.50, 0.30},
            {0.10, 0.20, 0.70},
            {0.05, 0.15, 0.80},
            {0.00, 0.00, 1.00},
        };
        std::vector<std::string> names = {
            "s100_m0_l0", "s80_m20_l0", "s70_m20_l10", "s60_m30_l10",
            "s50_m30_l20", "s33_m34_l33", "s20_m50_l30",
            "s10_m20_l70", "s5_m15_l80", "s0_m0_l100",
        };
        for (int i = 0; i < (int)mixes.size(); ++i) {
            auto p = make_default();
            p.category              = "7_task_type";
            auto& [s,m,l]           = mixes[i];
            p.pct_small             = s;
            p.pct_medium            = m;
            p.pct_large             = l;
            p.total_duration_budget = TOTAL_BUDGET;
            p.experiment_name       = "tasktype_" + names[i];
            all.push_back(p);
        }
    }

    // ─────────────────────────────────────────────
    // 8. WARUNEK STOPU
    // ─────────────────────────────────────────────
    {
        for (double mi : {10.0, 1.0, 0.1}) {
            auto p = make_default();
            p.category            = "8_stop_condition";
            p.stop_condition      = "temperature";
            p.initial_temperature = 100.0;
            p.min_temperature     = mi;
            p.experiment_name     = "stop_temperature_" + std::to_string(mi);
            all.push_back(p);
        }
        for (long long ni : {100LL,500LL,1000LL,10'000LL,20'000LL,30'000LL}) {
            auto p = make_default();
            p.category            = "8_stop_condition";
            p.stop_condition      = "no_improve";
            p.no_improve_limit    = ni;
            p.initial_temperature = 100.0;
            p.min_temperature     = 0.1;
            p.experiment_name     = "stop_no_improve_" + std::to_string(ni);
            all.push_back(p);
        }
        for (long long mi : {20'000LL,30'000LL,40'000LL,50'000LL}) {
            auto p = make_default();
            p.category             = "8_stop_condition";
            p.stop_condition       = "iterations";
            p.max_total_iterations = mi;
            p.initial_temperature  = 100.0;
            p.min_temperature      = 10.0;
            p.experiment_name      = "stop_iter_limit_" + std::to_string(mi);
            all.push_back(p);
        }
    }

    return all;
}

#endif // EXPERIMENT_CONFIG_H