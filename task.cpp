#include "task.h"
#include <algorithm> // dla std::max

Task::Task(int id, int duration, int priority, int due_time, int release_time){
    this->task_id = id;
    this->task_duration = duration;
    this->task_priority = priority;
    this->due_time = due_time;
    this->release_time = release_time;
}

Task::~Task(){
    // Destruktor, jeśli potrzebujesz zwolnić zasoby, możesz to zrobić tutaj
}

