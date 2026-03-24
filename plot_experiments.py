"""
plot_experiments.py
--------------------
Wczytuje wszystkie pliki CSV z katalogu results/ i generuje wykresy.
Obsługuje format uśredniony (NUM_RUNS powtórzeń):
  Kolumny: Iteration, Avg_Cost, Std_Cost, Avg_Temperature

Struktura wykresy/:
  wykresy/
    1_cooling_rate/
      cooling_85_individual.png
      ...
      _comparison_cost.png
      _comparison_temp.png
      _comparison_convergence.png
      _bar_best_costs.png
    ...
    _SUMMARY/
      all_best_costs.png
      best_per_category.png
      heatmap_best_costs.png

Użycie:
  python plot_experiments.py [katalog_danych] [katalog_wykresow]
  (domyślnie: ./results  ./wykresy)
"""

import os
import sys
import glob
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np

# ──────────────────────────────────────────────────────────────────────────────
#  Konfiguracja
# ──────────────────────────────────────────────────────────────────────────────
DATA_DIR     = sys.argv[1] if len(sys.argv) > 1 else "D:\\Pulpit\\PWR\\algorytmy optymalizacji\\results"
PLOT_DIR     = sys.argv[2] if len(sys.argv) > 2 else "D:\\Pulpit\\PWR\\algorytmy optymalizacji\\wykresy"
DPI          = 120
FIGSIZE_IND  = (12, 8)
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
    """Wczytuje CSV z nagłówkiem komentarzy (#). Zwraca (meta_dict, DataFrame)."""
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
    df.columns = [c.strip() for c in df.columns]

    # Ujednolicenie nazw kolumn – obsługa starego i nowego formatu
    rename = {}
    if "Cost" in df.columns and "Avg_Cost" not in df.columns:
        rename["Cost"] = "Avg_Cost"
    if "Temperature" in df.columns and "Avg_Temperature" not in df.columns:
        rename["Temperature"] = "Avg_Temperature"
    if rename:
        df = df.rename(columns=rename)
    if "Std_Cost" not in df.columns:
        df["Std_Cost"] = 0.0

    return meta, df


def best_cost_from(meta, df):
    """Wyciąga najlepszy koszt z metadanych lub z danych."""
    for key in ("avg_best_cost", "best_cost"):
        if key in meta:
            try:
                return int(float(meta[key]))
            except ValueError:
                pass
    return int(df["Avg_Cost"].min())


def num_runs_from(meta):
    return int(meta.get("num_runs", 1))


def nice_label(name):
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
#  Wykres indywidualny (jeden eksperyment, z pasmem ±1σ)
# ──────────────────────────────────────────────────────────────────────────────

def plot_individual(meta, df, exp_name, category, out_dir):
    best_cost  = best_cost_from(meta, df)
    std_bc     = meta.get("std_best_cost", "?")
    total_iter = meta.get("avg_total_iter", meta.get("total_iter", "?"))
    stop_cond  = meta.get("stop_cond", "?")
    n_runs     = num_runs_from(meta)

    iters = df["Iteration"].values
    cost  = df["Avg_Cost"].values
    std   = df["Std_Cost"].values
    temp  = df["Avg_Temperature"].values

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=FIGSIZE_IND, sharex=True)
    runs_label = f"{n_runs} runów" if n_runs > 1 else "1 run"
    fig.suptitle(
        f"{nice_label(exp_name)}\n"
        f"Śr. najlepszy koszt: {best_cost} ± {std_bc}  |  "
        f"Śr. iteracje: {total_iter}  |  Stop: {stop_cond}  |  {runs_label}",
        fontsize=11, fontweight="bold"
    )

    # -- koszt z pasmem std --
    ax1.plot(iters, cost, color="#2196F3", linewidth=1.4, label="Śr. koszt")
    ax1.fill_between(iters, cost - std, cost + std,
                     color="#2196F3", alpha=0.20, label="±1σ")
    ax1.set_ylabel("Koszt (weighted tardiness)")
    ax1.set_title("Uśredniony przebieg kosztu (pasmo = ±1 odch. std.)")
    ax1.grid(True, alpha=0.3)
    ax1.legend(fontsize=9)

    # -- temperatura --
    ax2.plot(iters, temp, color="#FF9800", linewidth=1.0, alpha=0.8, label="Śr. temperatura")
    ax2.set_ylabel("Temperatura")
    ax2.set_xlabel("Iteracja")
    ax2.set_title("Uśredniony przebieg temperatury")
    ax2.grid(True, alpha=0.3)
    ax2.legend(fontsize=9)

    plt.tight_layout()
    path = os.path.join(out_dir, category, f"{exp_name}_individual.png")
    save_fig(fig, path)


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres porównawczy – koszty (z pasmem ±1σ)
# ──────────────────────────────────────────────────────────────────────────────

def plot_comparison_cost(records, category, out_dir):
    cat_title = CATEGORY_TITLES.get(category, category)
    colors    = colormap_colors(len(records))
    n_runs    = num_runs_from(records[0][0]) if records else 1

    fig, ax = plt.subplots(figsize=FIGSIZE_COMP)
    ax.set_title(
        f"{cat_title}\n— Porównanie uśrednionego kosztu (pasmo ±1σ, {n_runs} runów)",
        fontsize=11, fontweight="bold"
    )

    for (meta, df, name), color in zip(records, colors):
        best = best_cost_from(meta, df)
        lbl  = f"{nice_label(name)}  (best≈{best})"
        iters = df["Iteration"].values
        cost  = df["Avg_Cost"].values
        std   = df["Std_Cost"].values

        ax.plot(iters, cost, label=lbl, color=color, linewidth=1.4, alpha=0.9)
        ax.fill_between(iters, cost - std, cost + std,
                        color=color, alpha=0.12)

    ax.set_xlabel("Iteracja")
    ax.set_ylabel("Koszt (weighted tardiness)")
    ax.legend(loc="upper right", fontsize=8, ncol=1)
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    save_fig(fig, os.path.join(out_dir, category, "_comparison_cost.png"))


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres porównawczy – temperatura
# ──────────────────────────────────────────────────────────────────────────────

def plot_comparison_temp(records, category, out_dir):
    cat_title = CATEGORY_TITLES.get(category, category)
    colors    = colormap_colors(len(records))

    fig, ax = plt.subplots(figsize=FIGSIZE_COMP)
    ax.set_title(cat_title + "\n— Porównanie przebiegu temperatury", fontsize=11, fontweight="bold")

    for (meta, df, name), color in zip(records, colors):
        ax.plot(df["Iteration"], df["Avg_Temperature"],
                label=nice_label(name), color=color, linewidth=1.0, alpha=0.75)

    ax.set_xlabel("Iteracja")
    ax.set_ylabel("Temperatura")
    ax.legend(loc="upper right", fontsize=8, ncol=1)
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    save_fig(fig, os.path.join(out_dir, category, "_comparison_temperature.png"))


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres konwergencji (znormalizowany koszt)
# ──────────────────────────────────────────────────────────────────────────────

def plot_comparison_convergence(records, category, out_dir):
    cat_title = CATEGORY_TITLES.get(category, category)
    colors    = colormap_colors(len(records))

    fig, ax = plt.subplots(figsize=FIGSIZE_COMP)
    ax.set_title(cat_title + "\n— Zbieżność (uśredniony koszt znormalizowany 0–1)",
                 fontsize=11, fontweight="bold")

    for (meta, df, name), color in zip(records, colors):
        costs = df["Avg_Cost"].values.astype(float)
        c_min, c_max = costs.min(), costs.max()
        norm = (costs - c_min) / (c_max - c_min) if c_max > c_min else np.zeros_like(costs)
        ax.plot(df["Iteration"], norm,
                label=nice_label(name), color=color, linewidth=1.2, alpha=0.85)

    ax.set_xlabel("Iteracja")
    ax.set_ylabel("Koszt (znormalizowany 0–1)")
    ax.legend(loc="upper right", fontsize=8, ncol=1)
    ax.grid(True, alpha=0.3)
    plt.tight_layout()
    save_fig(fig, os.path.join(out_dir, category, "_comparison_convergence.png"))


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres słupkowy z odchyleniem standardowym (errorbar)
# ──────────────────────────────────────────────────────────────────────────────

def plot_bar_best_costs(records, category, out_dir):
    cat_title = CATEGORY_TITLES.get(category, category)
    names  = [nice_label(r[2]) for r in records]
    costs  = []
    stds   = []
    for meta, df, name in records:
        costs.append(best_cost_from(meta, df))
        try:
            stds.append(float(meta.get("std_best_cost", 0)))
        except ValueError:
            stds.append(0.0)

    colors = colormap_colors(len(records))
    fig, ax = plt.subplots(figsize=(max(8, len(records) * 1.4 + 2), 6))
    bars = ax.bar(names, costs, color=colors, edgecolor="black", linewidth=0.5,
                  yerr=stds, capsize=4, error_kw={"elinewidth": 1.2, "ecolor": "#333"})

    ax.set_title(cat_title + "\n— Śr. najlepszy koszt ± odch. std. (słupki)",
                 fontsize=11, fontweight="bold")
    ax.set_ylabel("Śr. najlepszy koszt")
    ax.set_xlabel("Wariant")
    plt.xticks(rotation=30, ha="right", fontsize=8)

    for bar, val, std in zip(bars, costs, stds):
        label = f"{val}\n±{int(round(std))}" if std > 0 else str(val)
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() + max(costs) * 0.015,
                label, ha="center", va="bottom", fontsize=7)

    plt.tight_layout()
    save_fig(fig, os.path.join(out_dir, category, "_bar_best_costs.png"))


# ──────────────────────────────────────────────────────────────────────────────
#  Wykres skrzynkowy (box plot) – rozkład best_cost z 100 runów
#  Odtwarzamy go z avg ± std zakładając normalność (aproksymacja, bo nie
#  mamy surowych danych). Jeśli std == 0, rysujemy tylko punkt.
# ──────────────────────────────────────────────────────────────────────────────

def plot_box_approximation(records, category, out_dir):
    """
    Przybliżony box-plot: IQR ≈ 1.35σ, wąsy ≈ ±2σ.
    Używamy tylko gdy num_runs > 1.
    """
    n_runs = num_runs_from(records[0][0]) if records else 1
    if n_runs <= 1:
        return

    cat_title = CATEGORY_TITLES.get(category, category)
    names  = [nice_label(r[2]) for r in records]
    avgs   = []
    stds   = []
    for meta, df, name in records:
        avgs.append(best_cost_from(meta, df))
        try:
            stds.append(float(meta.get("std_best_cost", 0)))
        except ValueError:
            stds.append(0.0)

    # Symulujemy dane z rozkładu normalnego dla każdego wariantu
    rng = np.random.default_rng(42)
    data_for_box = []
    for avg, std in zip(avgs, stds):
        if std > 0:
            samples = rng.normal(avg, std, n_runs)
        else:
            samples = np.full(n_runs, avg)
        data_for_box.append(samples)

    colors = colormap_colors(len(records))
    fig, ax = plt.subplots(figsize=(max(8, len(records) * 1.4 + 2), 6))
    bp = ax.boxplot(data_for_box, patch_artist=True, notch=False,
                    medianprops={"color": "black", "linewidth": 1.5})

    for patch, color in zip(bp["boxes"], colors):
        patch.set_facecolor(color)
        patch.set_alpha(0.7)

    ax.set_xticks(range(1, len(names) + 1))
    ax.set_xticklabels(names, rotation=30, ha="right", fontsize=8)
    ax.set_title(
        cat_title + f"\n— Rozkład najlepszego kosztu (przybliżony, {n_runs} runów)",
        fontsize=11, fontweight="bold"
    )
    ax.set_ylabel("Najlepszy koszt")
    ax.grid(axis="y", alpha=0.3)
    plt.tight_layout()
    save_fig(fig, os.path.join(out_dir, category, "_boxplot_best_costs.png"))


# ──────────────────────────────────────────────────────────────────────────────
#  Podsumowanie globalne
# ──────────────────────────────────────────────────────────────────────────────

def plot_summary(all_records_by_cat, out_dir):
    summary_dir = os.path.join(out_dir, "_SUMMARY")
    os.makedirs(summary_dir, exist_ok=True)

    all_names, all_costs, all_stds, all_cats = [], [], [], []
    for cat, records in sorted(all_records_by_cat.items()):
        for meta, df, name in records:
            all_names.append(nice_label(name))
            all_costs.append(best_cost_from(meta, df))
            try:
                all_stds.append(float(meta.get("std_best_cost", 0)))
            except ValueError:
                all_stds.append(0.0)
            all_cats.append(cat)

    unique_cats = sorted(set(all_cats))
    cat_color   = {c: colormap_colors(len(unique_cats), "Set2")[i]
                   for i, c in enumerate(unique_cats)}
    bar_colors  = [cat_color[c] for c in all_cats]

    # --- 1. Słupkowy zbiorczy z errorbarami ---
    fig, ax = plt.subplots(figsize=(max(14, len(all_names) * 0.5 + 2), 7))
    bars = ax.bar(all_names, all_costs, color=bar_colors,
                  edgecolor="black", linewidth=0.4,
                  yerr=all_stds, capsize=3,
                  error_kw={"elinewidth": 0.9, "ecolor": "#444"})
    ax.set_title("Podsumowanie – śr. najlepszy koszt wszystkich eksperymentów (±1σ)",
                 fontsize=13, fontweight="bold")
    ax.set_ylabel("Śr. najlepszy koszt")
    plt.xticks(rotation=45, ha="right", fontsize=6)
    ax.grid(axis="y", alpha=0.3)

    from matplotlib.patches import Patch
    legend_handles = [Patch(facecolor=cat_color[c], label=CATEGORY_TITLES.get(c, c))
                      for c in unique_cats]
    ax.legend(handles=legend_handles, fontsize=7, loc="upper right")
    plt.tight_layout()
    save_fig(fig, os.path.join(summary_dir, "all_best_costs.png"))

    # --- 2. Najlepszy przebieg per kategoria ---
    fig2, ax2 = plt.subplots(figsize=(14, 7))
    ax2.set_title("Podsumowanie – uśredniony przebieg najlepszego wariantu per kategoria",
                  fontsize=13, fontweight="bold")

    for cat, records in sorted(all_records_by_cat.items()):
        best_rec = min(records, key=lambda r: best_cost_from(r[0], r[1]))
        meta, df, name = best_rec
        label = CATEGORY_TITLES.get(cat, cat).split("–")[0].strip()
        color = cat_color.get(cat)
        iters = df["Iteration"].values
        cost  = df["Avg_Cost"].values
        std   = df["Std_Cost"].values
        ax2.plot(iters, cost, label=f"{label} → {name}", color=color, linewidth=1.5)
        ax2.fill_between(iters, cost - std, cost + std, color=color, alpha=0.10)

    ax2.set_xlabel("Iteracja")
    ax2.set_ylabel("Śr. koszt")
    ax2.legend(fontsize=7, loc="upper right")
    ax2.grid(True, alpha=0.3)
    plt.tight_layout()
    save_fig(fig2, os.path.join(summary_dir, "best_per_category.png"))

    # --- 3. Heatmapa best_cost ---
    heat_cats = {c: r for c, r in all_records_by_cat.items() if 2 <= len(r) <= 8}
    if heat_cats:
        max_variants = max(len(r) for r in heat_cats.values())
        n_cats = len(heat_cats)
        heat_data = np.full((n_cats, max_variants), np.nan)
        row_labels, col_labels_all = [], []

        for row_i, (cat, records) in enumerate(sorted(heat_cats.items())):
            row_labels.append(CATEGORY_TITLES.get(cat, cat).split("–")[0].strip())
            for col_i, (meta, df, name) in enumerate(records):
                heat_data[row_i, col_i] = best_cost_from(meta, df)
                if row_i == 0:
                    col_labels_all.append(name[:18])
        while len(col_labels_all) < max_variants:
            col_labels_all.append("")

        fig3, ax3 = plt.subplots(figsize=(max(10, max_variants * 1.5), max(4, n_cats * 0.8)))
        im = ax3.imshow(heat_data, aspect="auto", cmap="RdYlGn_r")
        ax3.set_xticks(range(max_variants))
        ax3.set_xticklabels(col_labels_all, rotation=30, ha="right", fontsize=7)
        ax3.set_yticks(range(n_cats))
        ax3.set_yticklabels(row_labels, fontsize=8)
        ax3.set_title("Heatmapa śr. najlepszych kosztów (czerwony=gorszy, zielony=lepszy)",
                      fontsize=11, fontweight="bold")
        plt.colorbar(im, ax=ax3, label="Koszt")
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

    csv_files = sorted(glob.glob(os.path.join(DATA_DIR, "**", "*.csv"), recursive=True))
    if not csv_files:
        print("Brak plików CSV w podanym katalogu.")
        sys.exit(1)
    print(f"Znaleziono {len(csv_files)} plików CSV.\n")

    all_records_by_cat = {}

    for filepath in csv_files:
        category = os.path.basename(os.path.dirname(filepath))
        exp_name = os.path.splitext(os.path.basename(filepath))[0]

        try:
            meta, df = read_csv_with_meta(filepath)
        except Exception as e:
            print(f"  [WARN] Nie mozna wczytac {filepath}: {e}")
            continue

        if df.empty or "Avg_Cost" not in df.columns:
            print(f"  [WARN] Pusty lub nieprawidlowy CSV: {filepath}")
            continue

        print(f"  Indywidualny: {category}/{exp_name}")
        plot_individual(meta, df, exp_name, category, PLOT_DIR)

        all_records_by_cat.setdefault(category, []).append((meta, df, exp_name))

    print("\nGenerowanie wykresów porównawczych per kategoria...\n")
    for category, records in sorted(all_records_by_cat.items()):
        print(f"  Kategoria: {category}  ({len(records)} eksperymentów)")
        plot_comparison_cost       (records, category, PLOT_DIR)
        plot_comparison_temp       (records, category, PLOT_DIR)
        plot_comparison_convergence(records, category, PLOT_DIR)
        plot_bar_best_costs        (records, category, PLOT_DIR)
        plot_box_approximation     (records, category, PLOT_DIR)

    print("\nGenerowanie podsumowania globalnego...\n")
    plot_summary(all_records_by_cat, PLOT_DIR)

    print("\n=== Gotowe! ===")
    print(f"Wszystkie wykresy zapisane w: {PLOT_DIR}")


if __name__ == "__main__":
    main()