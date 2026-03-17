#ifndef TASK_GENERATOR_H
# define TASK_GENERATOR_H

#include "task.h"
#include <vector>


class TaskGenerator {
public:
    int num_tasks; // liczba zadań do wygenerowania
    int max_duration; // maksymalny czas trwania zadania
    int max_priority; // maksymalny priorytet zadania
    int max_due_time; // maksymalny czas do ukończenia zadania
    int max_release_time; // maksymalny czas, kiedy zadanie jest gotowe do wykonania
    

    std::vector<Task> generate_tasks(int num_tasks, int max_duration, int max_priority, int max_due_time, int max_release_time);
};


#endif