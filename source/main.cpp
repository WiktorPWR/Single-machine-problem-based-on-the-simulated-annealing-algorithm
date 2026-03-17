//g++ .\source\main.cpp .\source\simulated_annealing.cpp .\source\task.cpp .\source\task_generator.cpp .\source\thread_function.cpp -o main -I.\header

#include "main.h"   

const int NUM_TASKS = 100;
const int MAX_DURATION = 100;
const int MAX_PRIORITY = 10;
const int MAX_DUE_TIME = 100;
const int MAX_RELEASE_TIME = 100;

const int NUM_THREADS = 4; // Liczba wątków do uruchomienia

std::mutex result_mutex; 
std::vector<int> best_costs;

int main() {
    TaskGenerator generator;
    std::vector<Task> tasks = generator.generate_tasks(NUM_TASKS, MAX_DURATION, MAX_PRIORITY, MAX_DUE_TIME, MAX_RELEASE_TIME);

    std::vector<std::thread> threads;
    for(int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(thread_simulated_annealing_function, std::ref(tasks), i);
    }
    for(auto& thread : threads) {
        thread.join();
    }

    int global_best = *std::min_element(best_costs.begin(), best_costs.end());
    std::cout << "Globalnie najlepszy koszt: " << global_best << "\n";

    return 0;
}