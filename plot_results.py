import pandas as pd
import matplotlib.pyplot as plt

# Wczytanie danych
try:
    # Upewnij się, że ścieżka jest poprawna lub plik jest w tym samym folderze
    data = pd.read_csv('output_data.csv')
except FileNotFoundError:
    print("Nie znaleziono pliku output_data.csv. Uruchom najpierw program w C++.")
    exit()

# Znalezienie najlepszego wyniku (minimum w kolumnie Cost)
best_cost = data['Cost'].min()
final_cost = data['Cost'].iloc[-1] # Koszt w ostatniej iteracji

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 10), sharex=True)

# --- Wykres 1: Koszt ---
ax1.plot(data['Iteration'], data['Cost'], color='blue', label='Current Best Cost')

# Dodanie poziomej linii dla najlepszego wyniku
ax1.axhline(y=best_cost, color='green', linestyle='--', alpha=0.7, label=f'Best: {best_cost}')

# Adnotacja tekstowa na wykresie
ax1.annotate(f'Najlepszy wynik: {best_cost}', 
             xy=(data['Iteration'].iloc[-1], best_cost), 
             xytext=(data['Iteration'].iloc[-1]*0.7, best_cost + (data['Cost'].max()-best_cost)*0.1),
             arrowprops=dict(facecolor='black', shrink=0.05, width=1, headwidth=5),
             fontsize=12, fontweight='bold', color='green')

ax1.set_title('Zmiana kosztu w czasie (Simulated Annealing)')
ax1.set_ylabel('Koszt (Cost)')
ax1.grid(True, linestyle=':', alpha=0.6)
ax1.legend()

# --- Wykres 2: Temperatura ---
ax2.plot(data['Iteration'], data['Temperature'], color='red', label='Temperature')
ax2.set_title('Zmiana temperatury (Cooling & Reheating)')
ax2.set_xlabel('Iteracja całkowita')
ax2.set_ylabel('Temperatura (T)')
ax2.grid(True, linestyle=':', alpha=0.6)
ax2.legend()

# Wyświetlenie podsumowania w konsoli
print("-" * 30)
print(f"ANALIZA WYNIKÓW:")
print(f"Początkowy koszt: {data['Cost'].iloc[0]}")
print(f"Najlepszy znaleziony koszt: {best_cost}")
print(f"Liczba iteracji: {data['Iteration'].max()}")
print("-" * 30)

plt.tight_layout()
plt.show()