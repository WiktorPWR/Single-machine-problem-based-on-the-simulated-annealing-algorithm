cmake_minimum_required(VERSION 3.10)
project(SimulatedAnnealingProject)

# Ustawiamy standard C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dodajemy folder z nagłówkami, żebyś mógł pisać #include "main.h"
include_directories(header)

# Szukamy biblioteki wątków (odpowiednik -pthread)
find_package(Threads REQUIRED)

# Wskazujemy wszystkie pliki źródłowe
set(SOURCES 
    source/main.cpp 
    source/simulated_annealing.cpp 
    source/task.cpp 
    source/task_generator.cpp 
    source/thread_function.cpp
)

# Tworzymy plik wykonywalny
add_executable(main ${SOURCES})

# Linkujemy bibliotekę wątków do naszego programu
target_link_libraries(main PRIVATE Threads::Threads)