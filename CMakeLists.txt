cmake_minimum_required(VERSION 3.10)
project(SimulatedAnnealingProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(header)

find_package(Threads REQUIRED)

set(SOURCES 
    source/main_experiments.cpp    # <- zamieniony main
    source/simulated_annealing.cpp 
    source/task.cpp 
    source/task_generator.cpp 
    source/thread_function.cpp
    # source/thread_function.cpp możesz zostawić lub wykomentować
    # - run_experiments go nie używa bezpośrednio
)

add_executable(main ${SOURCES})

target_link_libraries(main PRIVATE Threads::Threads)
# ```

# **Struktura folderów powinna wyglądać tak:**
# ```
# projekt/
#   header/
#     config.h
#     main.h
#     task.h
#     task_generator.h
#     simulated_annealing.h
#     thread_function.h
#     experiment_config.h      ← nowy
#     experiment_runner.h      ← nowy
#   source/
#     main.cpp                 ← oryginalny (zostaw, nie używany)
#     main_experiments.cpp     ← nowy, ten kompilujesz
#     simulated_annealing.cpp
#     task.cpp
#     task_generator.cpp
#     thread_function.cpp
#   plot_experiments.py        ← nowy, w głównym folderze projektu
#   CMakeLists.txt