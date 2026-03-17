#include "simulated_annealing.h"

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

std::vector<Task> generate_neighbor(std::vector<Task>& current_solution) {
    // Placeholder for neighbor generation logic
    std::vector<Task> neighbor = current_solution;
    // Implement logic to modify the neighbor solution, e.g., swap two tasks, change task order, etc.
    int i = rand() % neighbor.size();
    int j;
    do { j = rand() % neighbor.size(); } while(j == i);
    std::swap(neighbor[i], neighbor[j]);


    return neighbor;
}