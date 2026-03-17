import pandas as pd
import matplotlib.pyplot as plt

# Wczytanie danych z C++
data = pd.read_csv('output_data.csv')

plt.figure(figsize=(10, 6))
plt.plot(data['Iteration'], data['Cost'], label='Koszt (SA)')
plt.title('Zmiana kosztu w czasie (Simulated Annealing)')
plt.xlabel('Iteracja')
plt.ylabel('Koszt')
plt.grid(True)
plt.legend()
plt.show()