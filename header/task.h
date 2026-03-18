#ifndef TASK_H
#define TASK_H

#include <vector>
#include <algorithm>
#include "config.h"   // stałe konfiguracyjne (MAX_DURATION itp.)

enum TaskType{
    SMALL_TASK,
    MEDIUM_TASK,
    LARGE_TASK
};

class Task{
public:
    int task_id; // unikalny identyfikator zadania
    int task_duration; // ile czasu zajmuje wykonanie zadania
    int task_priority; // im wiekszy tym ważniejsze
    int due_time; // kiedy ma byc zadnie skonczone
    int release_time; // kiedy zadanie jest gotowe do wykonania

    int setup_time; // czas potrzebny na przygotowanie maszyny do wykonania zadania
    int cleanup_time; // czas potrzebny na posprzątanie po wykonaniu zadania
    TaskType task_type; // typ zadania (małe, średnie, duże)    

    Task(int id, int duration, int priority, int due_time, int release_time);
    ~Task();

};

#endif