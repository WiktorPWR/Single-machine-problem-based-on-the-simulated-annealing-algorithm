#include "task_generator.h"
#include <cstdlib> // dla rand() i srand()

std::vector<Task> TaskGenerator::generate_tasks(int num_tasks, int max_duration, int max_priority, int max_due_time, int max_release_time) {
    std::vector<Task> tasks;
    for (int i = 0; i < num_tasks; ++i) {
        int duration = rand() % max_duration + 1; // losowy czas trwania od 1 do max_duration
        int priority = rand() % max_priority + 1; // losowy priorytet od 1 do max_priority
        int due_time = rand() % max_due_time + 1; // losowy czas do ukończenia od 1 do max_due_time
        int release_time = rand() % max_release_time + 1; // losowy czas gotowości od 1 do max_release_time
        tasks.emplace_back(i, duration, priority, due_time, release_time);
    }
    return tasks;
}