#include "task.h"
#include <algorithm> // dla std::max

Task::Task(int id, int duration, int priority, int due_time, int release_time){
    this->task_id = id;
    this->task_duration = duration;
    this->task_priority = priority;
    this->due_time = due_time;
    this->release_time = release_time;
    this->start_time = 0;
    this->end_time = 0;
    this->remaining_time = duration;
    this->is_completed = false;
}

Task::~Task(){
    // Destruktor, jeśli potrzebujesz zwolnić zasoby, możesz to zrobić tutaj
}

int Task::update_remaining_time(int current_time){
    if (current_time >= this->release_time && !this->is_completed) {
        int time_passed = current_time - this->start_time;
        this->remaining_time = std::max(0, this->task_duration - time_passed);
        if (this->remaining_time == 0) {
            this->is_completed = true;
            this->end_time = current_time;
        }
    }
    return this->remaining_time;
}


int Task::cost_calculation_time(){
    int lateness_penalty = std::max(0, this->end_time - this->due_time);
    return this->task_duration + lateness_penalty * this->task_priority;
}