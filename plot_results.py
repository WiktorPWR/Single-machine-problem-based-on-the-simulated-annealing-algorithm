import pandas as pd
import matplotlib.pyplot as plt
import glob

# Szukamy wszystkich plików wygenerowanych przez wątki
path = r'D:\Pulpit\PWR\algorytmy optymalizacji\data_output\output_thread_*.csv'
files = glob.glob(path)

if not files:
    print("Brak plików danych. Uruchom program w C++.")
    exit()

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10), sharex=True)

for file in files:
    data = pd.read_csv(file)
    thread_label = file.replace('.csv', '').replace('output_', '')
    
    # Wykres kosztu (wszystkie wątki na jednym)
    ax1.plot(data['Iteration'], data['Cost'], label=f'{thread_label} (Best: {data["Cost"].min()})', alpha=0.7)
    
    # Wykres temperatury
    ax2.plot(data['Iteration'], data['Temperature'], alpha=0.5)

# Formatowanie
ax1.set_title('Porównanie kosztów wszystkich wątków')
ax1.set_ylabel('Koszt')
ax1.legend(loc='upper right', fontsize='small', ncol=2)
ax1.grid(True, alpha=0.3)

ax2.set_title('Przebieg temperatury (Cooling & Reheating)')
ax2.set_ylabel('Temperatura')
ax2.set_xlabel('Iteracja')
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()