#ifndef CONFIG_H
#define CONFIG_H

#include <array>

// === Parametry zadań ===
const int MAX_DURATION    = 100;
const int MAX_PRIORITY    = 10;
const int MAX_DUE_TIME    = 100;
const int MAX_RELEASE_TIME = 100;

// =====================================================
//  Globalna macierz czasów przezbrojenia
//  Indeksy typów: 0 = SMALL_TASK, 1 = MEDIUM_TASK, 2 = LARGE_TASK
//  setup_matrix[from_type][to_type] = czas przezbrojenia
//
//  Domyślne wartości (identyczne z dotychczasowym task.cpp):
//    SMALL  → *  :  5
//    MEDIUM → *  : 10
//    LARGE  → *  : 20
//  (niezależnie od zadania docelowego – poprzednie zachowanie)
//
//  Przed każdym eksperymentem experiment_runner.h nadpisuje
//  tę macierz wartościami z ExperimentParams::setup_matrix.
// =====================================================
inline std::array<std::array<double, 3>, 3> SETUP_MATRIX = {{
    //         → SMALL   → MEDIUM  → LARGE
    /* SMALL  */ {  5.0,     5.0,     5.0 },
    /* MEDIUM */ { 10.0,    10.0,    10.0 },
    /* LARGE  */ { 20.0,    20.0,    20.0 }
}};

// Wygodna funkcja do ustawiania macierzy przed eksperymentem
inline void set_setup_matrix(
    double ss, double sm, double sl,
    double ms, double mm, double ml,
    double ls, double lm, double ll)
{
    SETUP_MATRIX[0] = { ss, sm, sl };
    SETUP_MATRIX[1] = { ms, mm, ml };
    SETUP_MATRIX[2] = { ls, lm, ll };
}

// Resetuje macierz do wartości domyślnych (per-typ, bez zróżnicowania celu)
inline void reset_setup_matrix() {
    set_setup_matrix(
         5.0,  5.0,  5.0,
        10.0, 10.0, 10.0,
        20.0, 20.0, 20.0
    );
}

#endif