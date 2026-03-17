#include "simulated_annealing.h"
#include <cmath>
#include <algorithm> // dla std::swap

int evaluate_solution(std::vector<Task>& tasks) {
    int total_cost = 0;
    int current_time = 0;

    for (auto& task : tasks) {
        current_time = std::max(current_time, task.release_time);
        current_time += task.task_duration;

        int lateness = std::max(0, current_time - task.due_time);
        total_cost += task.task_priority * lateness;
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
        // Wybieramy zadanie i wstawiamy je w zupełnie inne, losowe miejsce
        int from = rand() % n;
        int to = rand() % n;
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
        // --- BARDZO MAŁA ZMIANA (Fine-tuning): Zamiana tylko jeśli są blisko ---
        // Można też po prostu zostawić prosty swap sąsiadów
        int i = rand() % (n - 1);
        std::swap(neighbor[i], neighbor[i + 1]);
    }

    return neighbor;
}