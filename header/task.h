#ifndef MACHINE_H
# define MACHINE_H

class Task{
public:
    int task_id; // unikalny identyfikator zadania
    int task_duration; // ile czasu zajmuje wykonanie zadania
    int task_priority; // im wiekszy tym ważniejsze
    int due_time; // kiedy ma byc zadnie skonczone
    int release_time; // kiedy zadanie jest gotowe do wykonania

    Task(int id, int duration, int priority, int due_time, int release_time);
    ~Task();

};

#endif