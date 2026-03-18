# Szeregowanie zadań — Symulowane wyżarzanie (Simulated Annealing)

Projekt implementuje algorytm **symulowanego wyżarzania** do rozwiązywania problemu szeregowania zadań na jednej maszynie z minimalizacją ważonego opóźnienia. Algorytm uruchamiany jest równolegle w wielu wątkach, a wyniki każdego wątku zapisywane są do osobnych plików CSV.

---

## Problem

Mamy zbiór `n` zadań, które należy uszeregować na jednej maszynie. Każde zadanie charakteryzuje się:

| Parametr | Opis |
|---|---|
| `task_duration` | Czas wykonania zadania |
| `task_priority` | Waga/priorytet (im wyższy, tym droższe opóźnienie) |
| `due_time` | Termin zakończenia |
| `release_time` | Najwcześniejszy możliwy moment rozpoczęcia |
| `setup_time` | Czas przezbrojenia maszyny **przed** wykonaniem zadania |
| `cleanup_time` | Czas sprzątania **po** wykonaniu zadania (nie wpływa na ocenę opóźnienia) |

### Funkcja kosztu

Dla danego uszeregowania czas wykonania każdego zadania wyliczany jest kolejno:

```
current_time = max(current_time, release_time)
current_time += setup_time
current_time += task_duration
lateness = max(0, current_time - due_time)   ← spóźnienie mierzone tutaj
current_time += cleanup_time
```

Całkowity koszt rozwiązania:

```
koszt = Σ (priority_i × lateness_i)
```

Celem jest znalezienie takiej permutacji zadań, która **minimalizuje** ten koszt.

---

## Algorytm

### Symulowane wyżarzanie

Algorytm inspirowany procesem wyżarzania metali — na początku akceptuje gorsze rozwiązania z wysokim prawdopodobieństwem (eksploracja przestrzeni), a wraz ze spadkiem temperatury staje się coraz bardziej zachłanny (eksploatacja najlepszego obszaru).

**Schemat działania jednego wątku:**

```
T = T_initial
dopóki T > T_min:
    dla każdej iteracji wewnętrznej:
        wygeneruj sąsiada bieżącego rozwiązania
        oblicz deltę kosztu
        jeśli delta < 0 → zawsze akceptuj
        jeśli delta >= 0 → akceptuj z prawdopodobieństwem exp(-delta / T)
    
    dostosuj temperaturę na podstawie acceptance_rate
```

### Adaptacyjne chłodzenie

Tempo chłodzenia dobierane jest dynamicznie na podstawie współczynnika akceptacji w danej iteracji zewnętrznej:

| Sytuacja | Działanie |
|---|---|
| `acceptance_rate == 0` | **Reheating** — podgrzanie temperatury (max 5 razy) |
| `acceptance_rate < 0.4` | Wolne chłodzenie (`×0.99`) |
| `0.4 ≤ acceptance_rate ≤ 0.6` | Normalne chłodzenie (`×0.95`) |
| `acceptance_rate > 0.6` | Szybkie chłodzenie (`×0.91`) |

### Generowanie sąsiedztwa

Ruch w przestrzeni rozwiązań dobierany jest adaptacyjnie do aktualnej temperatury:

| Temperatura | Typ ruchu | Opis |
|---|---|---|
| `T > 80` | **2-opt reverse** | Odwrócenie losowego podciągu zadań — duża zmiana |
| `T > 60` | **Insertion move** | Wyjęcie zadania i wstawienie w inne miejsce — średnia zmiana |
| `T > 40` | **Swap sąsiadów** | Zamiana dwóch sąsiednich zadań — mała zmiana |
| `T ≤ 40` | **Swap sąsiadów** | Fine-tuning — bardzo mała zmiana |

### Wielowątkowość

Program uruchamia `NUM_THREADS` niezależnych wątków, z których każdy:
- otrzymuje ten sam zestaw zadań jako punkt startowy,
- korzysta z własnego generatora liczb losowych (ziarno = czas + `thread_id`),
- przeszukuje przestrzeń rozwiązań niezależnie,
- zapisuje historię kosztu i temperatury do pliku CSV,
- po zakończeniu zapisuje swój najlepszy wynik do wspólnego wektora chronionego mutexem.

Na końcu wybierany jest **globalnie najlepszy** wynik spośród wszystkich wątków.

---

## Typy zadań

Zadania dzielone są na trzy kategorie na podstawie czasu trwania:

| Typ | Próg (`task_duration`) | `setup_time` | `cleanup_time` |
|---|---|---|---|
| `SMALL_TASK` | `< 50` | 5 s | 3 s |
| `MEDIUM_TASK` | `50 – 79` | 10 s | 3 s |
| `LARGE_TASK` | `≥ 80` | 20 s | 3 s |

---

## Parametry konfiguracyjne

Wszystkie stałe konfiguracyjne zebrane są w dwóch plikach nagłówkowych:

**`config.h`** — parametry generatora zadań:
```cpp
MAX_DURATION     = 100   // maksymalny czas trwania zadania
MAX_PRIORITY     = 10    // maksymalny priorytet
MAX_DUE_TIME     = 100   // maksymalny termin
MAX_RELEASE_TIME = 100   // maksymalny czas gotowości
```

**`simulated_annealing.h`** — parametry algorytmu:
```cpp
INITIAL_TEMPERATURE = 100.0
MIN_TEMPERATURE     = 0.2
MAX_ITERATIONS      = 10      // iteracji wewnętrznych na krok temperatury
COOLING_RATE_NORMAL = 0.95
COOLING_RATE_FAST   = 0.91
COOLING_RATE_SLOW   = 0.99
REHEAT_FACTOR       = 1.5
MAX_REHEAT_COUNT    = 5
```

**`main.cpp`** — parametry uruchomienia:
```cpp
NUM_TASKS   = 100   // liczba zadań
NUM_THREADS = 10    // liczba równoległych wątków
```

---

## Struktura projektu

```
├── header/
│   ├── config.h                 # stałe konfiguracyjne
│   ├── main.h
│   ├── task.h                   # klasa Task
│   ├── task_generator.h
│   ├── simulated_annealing.h    # stałe SA + deklaracje funkcji
│   └── thread_function.h
├── source/
│   ├── main.cpp                 # punkt wejścia, uruchomienie wątków
│   ├── task.cpp                 # konstruktor Task, klasyfikacja typu
│   ├── task_generator.cpp       # losowe generowanie zestawu zadań
│   ├── simulated_annealing.cpp  # evaluate_solution, generate_neighbor
│   └── thread_function.cpp      # główna pętla SA, zapis CSV
└── README.md
```

---

## Kompilacja

```bash
g++ source/main.cpp source/simulated_annealing.cpp source/task.cpp \
    source/task_generator.cpp source/thread_function.cpp \
    -o main -I./header -lpthread
```

Wymagania: kompilator zgodny z **C++17** (użycie `std::filesystem`).

---

## Wyniki

Po uruchomieniu każdy wątek tworzy plik `output_thread_<id>.csv` z historią optymalizacji:

```
Iteration,Cost,Temperature
0,48320,100.0
1,47891,100.0
...
```

Na standardowym wyjściu wypisywany jest wynik każdego wątku oraz globalnie najlepszy koszt:

```
Wątek 0 zakończył. Najlepszy koszt: 43210
Wątek 1 zakończył. Najlepszy koszt: 41890
...
Globalnie najlepszy koszt: 41890
```

---

## Możliwe rozszerzenia

- Implementacja kryterium stopu opartego o brak poprawy przez `k` iteracji
- Wymiana rozwiązań między wątkami (algorytm wyspowy)
- Parametryzacja przez argumenty wiersza poleceń zamiast stałych w kodzie
- Wizualizacja zbieżności na podstawie danych CSV