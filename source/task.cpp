#include "task.h"


const double SMALL_TASK_THRESHOLD = 0; // Przykładowy próg dla małych zadań
const double MEDIUM_TASK_THRESHOLD = MAX_DURATION * 0.5; // Przykładowy próg dla średnich zadań
const double LARGE_TASK_THRESHOLD = MAX_DURATION * 0.8; // Przykładowy próg dla dużych zadań

const int SETUP_TIME_SMALL = 5;
const int SETUP_TIME_MEDIUM = 10;
const int SETUP_TIME_LARGE = 20;

const int CLEANUP_TIME_SMALL = 3;

Task::Task(int id, int duration, int priority, int due_time, int release_time){
    this->task_id = id;
    this->task_duration = duration;
    this->task_priority = priority;
    this->due_time = due_time;
    this->release_time = release_time;
    this->cleanup_time = CLEANUP_TIME_SMALL; // Przykładowa stała wartość dla czasu sprzątania
    // Określenie typu zadania na podstawie czasu trwania
    if(duration >= LARGE_TASK_THRESHOLD) {
        this->task_type = LARGE_TASK;
        this->setup_time = SETUP_TIME_LARGE;
    } else if(duration >= MEDIUM_TASK_THRESHOLD && duration < LARGE_TASK_THRESHOLD) {
        this->task_type = MEDIUM_TASK;
        this->setup_time = SETUP_TIME_MEDIUM;
    } else {
        this->task_type = SMALL_TASK;
        this->setup_time = SETUP_TIME_SMALL;
    }


}

Task::~Task(){
    // Destruktor, jeśli potrzebujesz zwolnić zasoby, możesz to zrobić tutaj
}

