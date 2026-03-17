#ifndef MACHINE_H
# define MACHINE_H

class Task{
public:
    int task_id; // unikalny identyfikator zadania
    int task_duration; // ile czasu zajmuje wykonanie zadania
    int task_priority; // im wiekszy tym ważniejsze
    int due_time; // kiedy ma byc zadnie skonczone
    int release_time; // kiedy zadanie jest gotowe do wykonania

    int start_time; // kiedy zadanie zostalo rozpoczęte
    int end_time; // kiedy zadanie zostalo zakończone
    int remaining_time; // ile czasu pozostało do wykonania zadania
    bool is_completed; // czy zadanie zostało zakończone


    int update_remaining_time(int current_time);// aktualizuje remaining_time na podstawie current_time i zwraca aktualną wartość remaining_time

    int cost_calculation_time(); // oblicza i zwraca czas potrzebny do wykonania zadania, biorąc pod uwagę jego priorytet i czas trwania oraz kare za spóźnienie

    Task(int id, int duration, int priority, int due_time, int release_time);
    ~Task();

};

#endif