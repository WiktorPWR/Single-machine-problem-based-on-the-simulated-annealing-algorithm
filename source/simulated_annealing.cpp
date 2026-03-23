#include "simulated_annealing.h"
#include "config.h"
#include <cmath>
#include <algorithm> // dla std::swap

int evaluate_solution(std::vector<Task>& tasks) {
    int total_cost   = 0;
    int current_time = 0;

    for (int i = 0; i < (int)tasks.size(); ++i) {
        auto& task = tasks[i];

        // Czekamy aż zadanie będzie gotowe do wykonania
        current_time = std::max(current_time, task.release_time);

        // ── Czas przezbrojenia z poprzedniego zadania na bieżące ──
        // Pierwsze zadanie: brak poprzednika → czas = 0
        // Kolejne zadania : SETUP_MATRIX[typ_poprzedniego][typ_bieżącego]
        if (i > 0) {
            int from_type = static_cast<int>(tasks[i - 1].task_type);
            int to_type   = static_cast<int>(task.task_type);
            current_time += static_cast<int>(SETUP_MATRIX[from_type][to_type]);
        }

        // Wykonanie zadania
        current_time += task.task_duration;

        // Spóźnienie liczymy po wykonaniu, przed cleanup
        int lateness = std::max(0, current_time - task.due_time);
        total_cost  += task.task_priority * lateness;

        // Sprzątanie po zadaniu (nie wpływa na lateness)
        current_time += task.cleanup_time;
    }
    return total_cost;
}

std::vector<Task> generate_neighbor(std::vector<Task>& current_solution, double temperature) {
    std::vector<Task> neighbor = current_solution;
    if (neighbor.size() < 2) return neighbor;

    int n = neighbor.size();

    if (temperature > HIGH_TEMPERATURE) {
        // --- BARDZO DUŻA ZMIANA: Odwrócenie losowego podciągu (2-opt move) ---
        int i = rand() % n;
        int j = rand() % n;
        if (i > j) std::swap(i, j);
        std::reverse(neighbor.begin() + i, neighbor.begin() + j + 1);

    } else if (temperature > MEDIUM_TEMPERATURE) {
        // --- ŚREDNIA ZMIANA: Przesunięcie (Insertion Move) ---
        int from = rand() % n;
        int to   = rand() % n;
        if (from != to) {
            Task task_to_move = neighbor[from];
            neighbor.erase(neighbor.begin() + from);
            neighbor.insert(neighbor.begin() + to, task_to_move);
        }

    } else if (temperature > LOW_TEMPERATURE) {
        // --- MAŁA ZMIANA: Zamiana dwóch sąsiadujących zadań ---
        int i = rand() % (n - 1);
        std::swap(neighbor[i], neighbor[i + 1]);

    } else {
        // --- BARDZO MAŁA ZMIANA (Fine-tuning) ---
        int i = rand() % (n - 1);
        std::swap(neighbor[i], neighbor[i + 1]);
    }

    return neighbor;
}