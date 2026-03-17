#include <iostream>
#include "task.h"
#include "task_generator.h"



const int NUM_TASKS = 10;
const int MAX_DURATION = 100;
const int MAX_PRIORITY = 10;
const int MAX_DUE_TIME = 100;
const int MAX_RELEASE_TIME = 100;



int main(){
    TaskGenerator generator;
    std::vector<Task> tasks = generator.generate_tasks(NUM_TASKS, MAX_DURATION, MAX_PRIORITY, MAX_DUE_TIME, MAX_RELEASE_TIME);


}