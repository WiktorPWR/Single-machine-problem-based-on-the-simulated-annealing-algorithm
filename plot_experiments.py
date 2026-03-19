"""
plot_experiments.py
--------------------
Wczytuje wszystkie pliki CSV z katalogu results/ i generuje:
  - Dla każdego eksperymentu: indywidualny wykres kosztu i temperatury (Cost+Temp)
  - Dla każdej kategorii: wykres porównawczy wszystkich przebiegów

Struktura wykresy/:
  wykresy/
    1_cooling_rate/
      cooling_85_individual.png
      cooling_91_individual.png
      ...
      _comparison_cost.png
      _comparison_temp.png
    2_acceptance_rate/
      ...
    ...
    _SUMMARY/
      best_costs_per_category.png
      all_best_costs.png

Użycie:
  python plot_experiments.py [katalog_danych] [katalog_wykresow]
  (domyślnie: ./results  ./wykresy)
"""

import os
import sys
import glob
import re
import pandas as pd
import matplotlib
matplotlib.use("Agg")  # bez GUI – zapis do pliku
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np

# ──────────────────────────────────────────────────────────────────────────────
#  Konfiguracja
# ──────────────────────────────────────────────────────────────────────────────
DATA_DIR   = sys.argv[1] if len(sys.argv) > 1 else "D:\\Pulpit\\PWR\\algorytmy optymalizacji\\results"
PLOT_DIR   = sys.argv[2] if len(sys.argv) > 2 else "D:\\Pulpit\\PWR\\algorytmy optymalizacji\\wykresy"
DPI        = 120
FIGSIZE_IND  = (12, 7)
FIGSIZE_COMP = (14, 8)

CATEGORY_TITLES = {
    "1_cooling_rate"      : "1. Cooling Rate – porównanie stałych współczynników chłodzenia",
    "2_acceptance_rate"   : "2. Acceptance Rate – progi adaptacyjnego chłodzenia",
    "3_setup_times"       : "3. Czasy przezbrojenia – wpływ miksu typów zadań",
    "4_iterations"        : "4. Liczba iteracji na temperaturę",
    "5_reheating"         : "5. Reheating – strategia podgrzewania temperatury",
    "6_start_temperature" : "6. Temperatura startowa",
    "7_task_type"         : "7. Typy zadań (miks procentowy Small/Medium/Large)",
    "8_stop_condition"    : "8. Warunki stopu algorytmu",
}

# ──────────────────────────────────────────────────────────────────────────────
#  Pomocniki
# ──────────────────────────────────────────────────────────────────────────────

def read_csv_with_meta(filepath):
    """Wczytuje CSV z nagłówkiem komentarzy (#) i zwraca (meta_dict, DataFrame)."""
    meta = {}
    skip = 0
    with open(filepath, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line.startswith("#"):
                skip += 1
                parts = line[1:].strip().split("=", 1)
                if len(parts) == 2:
                    meta[parts[0].strip()] = parts[1].strip()
            else:
                break
    df = pd.read_csv(filepath, skiprows=skip)
    return meta, df


def nice_label(name):
    """Zamienia snake_case na ładny label do legendy."""
    return name.replace("_", " ")


def colormap_colors(n, cmap_name="tab10"):
    cmap = cm.get_cmap(cmap_name, max(n, 2))
    return [cmap(i) for i in range(n)]


def save_fig(fig, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    fig.savefig(path, dpi=DPI, bbox_inches="tight")
    plt.close(fig)
    print(f"  Zapisano: {path}")


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres indywidualny (jeden eksperyment)
# ──────────────────────────────────────────────────────────────────────────────

def plot_individual(meta, df, exp_name, category, out_dir):
    best_cost = meta.get("best_cost", "?")
    total_iter = meta.get("total_iter", "?")
    stop_cond  = meta.get("stop_cond", "?")

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=FIGSIZE_IND, sharex=True)
    fig.suptitle(
        f"{nice_label(exp_name)}\n"
        f"Najlepszy koszt: {best_cost}  |  Iteracje: {total_iter}  |  Stop: {stop_cond}",
        fontsize=12, fontweight="bold"
    )

    ax1.plot(df["Iteration"], df["Cost"], color="#2196F3", linewidth=1.2, label="Koszt")
    ax1.set_ylabel("Koszt (weighted tardiness)")
    ax1.set_title("Przebieg kosztu")
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    ax2.plot(df["Iteration"], df["Temperature"], color="#FF9800", linewidth=1.0,
         alpha=0.7, label="Temperatura")
    ax2.set_ylabel("Temperatura")
    ax2.set_xlabel("Iteracja")
    ax2.set_title("Przebieg temperatury")
    ax2.grid(True, alpha=0.3)
    ax2.legend()

    plt.tight_layout()
    path = os.path.join(out_dir, category, f"{exp_name}_individual.png")
    save_fig(fig, path)


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres porównawczy kategorii – koszty
# ──────────────────────────────────────────────────────────────────────────────

def plot_comparison_cost(records, category, out_dir):
    """records: lista (meta, df, exp_name)"""
    cat_title = CATEGORY_TITLES.get(category, category)
    colors = colormap_colors(len(records))

    fig, ax = plt.subplots(figsize=FIGSIZE_COMP)
    ax.set_title(cat_title + "\n— Porównanie przebiegu kosztu", fontsize=12, fontweight="bold")

    for (meta, df, name), color in zip(records, colors):
        best = meta.get("best_cost", "?")
        label = f"{nice_label(name)}  (best={best})"
        ax.plot(df["Iteration"], df["Cost"],
                label=label, color=color, linewidth=1.2, alpha=0.85)

    ax.set_xlabel("Iteracja")
    ax.set_ylabel("Koszt (weighted tardiness)")
    ax.legend(loc="upper right", fontsize=8, ncol=1)
    ax.grid(True, alpha=0.3)
    plt.tight_layout()

    path = os.path.join(out_dir, category, "_comparison_cost.png")
    save_fig(fig, path)


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres porównawczy kategorii – temperatura
# ──────────────────────────────────────────────────────────────────────────────

def plot_comparison_temp(records, category, out_dir):
    cat_title = CATEGORY_TITLES.get(category, category)
    colors = colormap_colors(len(records))

    fig, ax = plt.subplots(figsize=FIGSIZE_COMP)
    ax.set_title(cat_title + "\n— Porównanie przebiegu temperatury", fontsize=12, fontweight="bold")

    for (meta, df, name), color in zip(records, colors):
        ax.plot(df["Iteration"], df["Temperature"],
                label=nice_label(name), color=color, linewidth=1.0, alpha=0.75)

    ax.set_xlabel("Iteracja")
    ax.set_ylabel("Temperatura")
    ax.legend(loc="upper right", fontsize=8, ncol=1)
    ax.grid(True, alpha=0.3)
    plt.tight_layout()

    path = os.path.join(out_dir, category, "_comparison_temperature.png")
    save_fig(fig, path)


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres porównawczy kategorii – konwergencja (normalizacja kosztu)
# ──────────────────────────────────────────────────────────────────────────────

def plot_comparison_convergence(records, category, out_dir):
    """Normalizowany koszt (0..1) względem iteracji – ułatwia porównanie zbieżności."""
    cat_title = CATEGORY_TITLES.get(category, category)
    colors = colormap_colors(len(records))

    fig, ax = plt.subplots(figsize=FIGSIZE_COMP)
    ax.set_title(cat_title + "\n— Zbieżność (koszt znormalizowany)", fontsize=12, fontweight="bold")

    for (meta, df, name), color in zip(records, colors):
        costs = df["Cost"].values.astype(float)
        c_min, c_max = costs.min(), costs.max()
        if c_max > c_min:
            norm = (costs - c_min) / (c_max - c_min)
        else:
            norm = np.zeros_like(costs)
        ax.plot(df["Iteration"], norm,
                label=nice_label(name), color=color, linewidth=1.2, alpha=0.85)

    ax.set_xlabel("Iteracja")
    ax.set_ylabel("Koszt (znormalizowany 0–1)")
    ax.legend(loc="upper right", fontsize=8, ncol=1)
    ax.grid(True, alpha=0.3)
    plt.tight_layout()

    path = os.path.join(out_dir, category, "_comparison_convergence.png")
    save_fig(fig, path)


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres słupkowy najlepszych kosztów w kategorii
# ──────────────────────────────────────────────────────────────────────────────

def plot_bar_best_costs(records, category, out_dir):
    cat_title = CATEGORY_TITLES.get(category, category)
    names  = [nice_label(r[2]) for r in records]
    costs  = []
    for meta, df, name in records:
        try:
            costs.append(int(meta["best_cost"]))
        except Exception:
            costs.append(int(df["Cost"].min()))

    colors = colormap_colors(len(records))
    fig, ax = plt.subplots(figsize=(max(8, len(records) * 1.2 + 2), 6))
    bars = ax.bar(names, costs, color=colors, edgecolor="black", linewidth=0.5)
    ax.set_title(cat_title + "\n— Najlepszy koszt (słupki)", fontsize=12, fontweight="bold")
    ax.set_ylabel("Najlepszy koszt")
    ax.set_xlabel("Wariant")
    plt.xticks(rotation=30, ha="right", fontsize=8)

    # wartości nad słupkami
    for bar, val in zip(bars, costs):
        ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() + max(costs) * 0.01,
                str(val), ha="center", va="bottom", fontsize=8)

    plt.tight_layout()
    path = os.path.join(out_dir, category, "_bar_best_costs.png")
    save_fig(fig, path)


# ──────────────────────────────────────────────────────────────────────────────
#  Podsumowanie globalne
# ──────────────────────────────────────────────────────────────────────────────

def plot_summary(all_records_by_cat, out_dir):
    """Jeden wykres zbiorczy: najlepszy koszt każdego eksperymentu, pogrupowany per kategorię."""
    summary_dir = os.path.join(out_dir, "_SUMMARY")
    os.makedirs(summary_dir, exist_ok=True)

    # --- 1. Słupkowy zbiorczy (wszystkie eksperymenty) ---
    all_names  = []
    all_costs  = []
    all_cats   = []
    for cat, records in sorted(all_records_by_cat.items()):
        for meta, df, name in records:
            try:
                c = int(meta["best_cost"])
            except Exception:
                c = int(df["Cost"].min())
            all_names.append(nice_label(name))
            all_costs.append(c)
            all_cats.append(cat)

    unique_cats = sorted(set(all_cats))
    cat_color   = {c: colormap_colors(len(unique_cats), "Set2")[i]
                   for i, c in enumerate(unique_cats)}

    bar_colors = [cat_color[c] for c in all_cats]
    fig, ax = plt.subplots(figsize=(max(14, len(all_names) * 0.45 + 2), 7))
    bars = ax.bar(all_names, all_costs, color=bar_colors, edgecolor="black", linewidth=0.4)
    ax.set_title("Podsumowanie – najlepszy koszt wszystkich eksperymentów",
                 fontsize=13, fontweight="bold")
    ax.set_ylabel("Najlepszy koszt")
    plt.xticks(rotation=45, ha="right", fontsize=6)
    ax.grid(axis="y", alpha=0.3)

    # Legenda kategorii
    from matplotlib.patches import Patch
    legend_handles = [Patch(facecolor=cat_color[c], label=CATEGORY_TITLES.get(c, c))
                      for c in unique_cats]
    ax.legend(handles=legend_handles, fontsize=7, loc="upper right")

    plt.tight_layout()
    save_fig(fig, os.path.join(summary_dir, "all_best_costs.png"))

    # --- 2. Jeden wykres z najlepszymi przebiegami per kategorii ---
    fig2, ax2 = plt.subplots(figsize=(14, 7))
    ax2.set_title("Podsumowanie – najlepszy przebieg kosztu per kategoria",
                  fontsize=13, fontweight="bold")

    for cat, records in sorted(all_records_by_cat.items()):
        # Wybierz rekord z najniższym best_cost
        best_rec = min(records, key=lambda r: int(r[0].get("best_cost", 9e9)))
        meta, df, name = best_rec
        label = CATEGORY_TITLES.get(cat, cat).split("–")[0].strip()
        color = cat_color.get(cat, None)
        ax2.plot(df["Iteration"], df["Cost"],
                 label=f"{label} → {name}", color=color, linewidth=1.4)

    ax2.set_xlabel("Iteracja")
    ax2.set_ylabel("Koszt")
    ax2.legend(fontsize=7, loc="upper right")
    ax2.grid(True, alpha=0.3)
    plt.tight_layout()
    save_fig(fig2, os.path.join(summary_dir, "best_per_category.png"))

    # --- 3. Heatmapa: kategoria × wariant → best_cost ---
    # Ograniczamy tylko do kategorii z >= 2 wariantami i <= 8
    heat_cats = {c: r for c, r in all_records_by_cat.items() if 2 <= len(r) <= 8}
    if heat_cats:
        max_variants = max(len(r) for r in heat_cats.values())
        n_cats = len(heat_cats)
        heat_data = np.full((n_cats, max_variants), np.nan)
        row_labels = []
        col_labels_all = []

        for row_i, (cat, records) in enumerate(sorted(heat_cats.items())):
            row_labels.append(CATEGORY_TITLES.get(cat, cat).split("–")[0].strip())
            for col_i, (meta, df, name) in enumerate(records):
                try:
                    heat_data[row_i, col_i] = int(meta["best_cost"])
                except Exception:
                    pass
                if row_i == 0:
                    col_labels_all.append(name[:18])

        # Dopełnij etykiety kolumn
        while len(col_labels_all) < max_variants:
            col_labels_all.append("")

        fig3, ax3 = plt.subplots(figsize=(max(10, max_variants * 1.5), max(4, n_cats * 0.8)))
        im = ax3.imshow(heat_data, aspect="auto", cmap="RdYlGn_r")
        ax3.set_xticks(range(max_variants))
        ax3.set_xticklabels(col_labels_all, rotation=30, ha="right", fontsize=7)
        ax3.set_yticks(range(n_cats))
        ax3.set_yticklabels(row_labels, fontsize=8)
        ax3.set_title("Heatmapa najlepszych kosztów (czerwony=gorszy, zielony=lepszy)",
                      fontsize=11, fontweight="bold")
        plt.colorbar(im, ax=ax3, label="Koszt")

        # Wartości w komórkach
        for i in range(n_cats):
            for j in range(max_variants):
                if not np.isnan(heat_data[i, j]):
                    ax3.text(j, i, str(int(heat_data[i, j])),
                             ha="center", va="center", fontsize=6, color="black")

        plt.tight_layout()
        save_fig(fig3, os.path.join(summary_dir, "heatmap_best_costs.png"))


# ──────────────────────────────────────────────────────────────────────────────
#  Main
# ──────────────────────────────────────────────────────────────────────────────

def main():
    if not os.path.isdir(DATA_DIR):
        print(f"BLAD: Katalog danych nie istnieje: {DATA_DIR}")
        sys.exit(1)

    print(f"Dane:    {DATA_DIR}")
    print(f"Wykresy: {PLOT_DIR}\n")

    # Zbierz wszystkie CSV
    pattern = os.path.join(DATA_DIR, "**", "*.csv")
    csv_files = sorted(glob.glob(pattern, recursive=True))

    if not csv_files:
        print("Brak plików CSV w podanym katalogu.")
        sys.exit(1)

    print(f"Znaleziono {len(csv_files)} plikow CSV.\n")

    # Pogrupuj per kategoria
    all_records_by_cat = {}

    for filepath in csv_files:
        # category = bezpośredni rodzic pliku CSV
        category = os.path.basename(os.path.dirname(filepath))
        exp_name = os.path.splitext(os.path.basename(filepath))[0]

        try:
            meta, df = read_csv_with_meta(filepath)
        except Exception as e:
            print(f"  [WARN] Nie mozna wczytac {filepath}: {e}")
            continue

        if df.empty or "Cost" not in df.columns:
            print(f"  [WARN] Pusty lub nieprawidlowy CSV: {filepath}")
            continue

        # Indywidualny wykres
        print(f"  Indywidualny: {category}/{exp_name}")
        plot_individual(meta, df, exp_name, category, PLOT_DIR)

        if category not in all_records_by_cat:
            all_records_by_cat[category] = []
        all_records_by_cat[category].append((meta, df, exp_name))

    print("\nGenerowanie wykresów porównawczych per kategoria...\n")
    for category, records in sorted(all_records_by_cat.items()):
        print(f"  Kategoria: {category}  ({len(records)} eksperymentow)")
        plot_comparison_cost       (records, category, PLOT_DIR)
        plot_comparison_temp       (records, category, PLOT_DIR)
        plot_comparison_convergence(records, category, PLOT_DIR)
        plot_bar_best_costs        (records, category, PLOT_DIR)

    print("\nGenerowanie podsumowania globalnego...\n")
    plot_summary(all_records_by_cat, PLOT_DIR)

    print("\n=== Gotowe! ===")
    print(f"Wszystkie wykresy zapisane w: {PLOT_DIR}")


if __name__ == "__main__":
    main()
