#!/usr/bin/env python3
"""
plot_results.py — Gera gráficos (PNG) e o relatório results/summary.md a partir
dos CSVs produzidos por `mlp_bench`.

Uso:  python3 scripts/plot_results.py [DIRETORIO_RESULTADOS]   (padrão: results/)

Saídas em DIRETORIO/figures/:
  fig1_exatos_tempo.png    força bruta vs DP + curvas teóricas
  fig2_qualidade_gap.png   gap das heurísticas vs ótimo (m ≤ 18)
  fig3_dp_vs_gils.png      tempo DP vs GILS-RVND (onde a heurística compensa)
  fig4_escala_tempo.png    tempo das heurísticas (log-log), GILS em dois regimes
  fig5_escala_gap.png      gap vs melhor solução conhecida (n até 200)
  fig6_tradeoff.png        custo × tempo variando I_ILS, com desvio-padrão
  fig7_dp_memoria.png      pico de memória da DP (medido) vs tabela teórica
(perturb.csv, se existir, vira uma tabela no relatório)
e DIRETORIO/summary.md, que começa por "Principais achados" — todos os números
citados no README e na apresentação saem daqui, nunca digitados à mão.

Compatível com CSVs antigos (sem peak_rss_mb em exact.csv, sem seed em
tradeoff.csv): o que faltar é simplesmente omitido.
Dependências: matplotlib, numpy (requirements.txt).
"""
import csv
import math
import sys
from collections import defaultdict
from pathlib import Path
from statistics import mean, median, pstdev

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import matplotlib.ticker as mticker  # noqa: E402
import numpy as np  # noqa: E402

# --------------------------------------------------------------------------- estilo
COLOR = {"dp": "#2a78d6", "brute": "#eb6834", "nn": "#1baf7a", "nn-rvnd": "#eda100", "gils": "#e34948"}
MARKER = {"dp": "o", "brute": "s", "nn": "^", "nn-rvnd": "D", "gils": "o"}
LABEL = {"dp": "DP sobre subconjuntos", "brute": "Força bruta", "nn": "Vizinho mais próximo",
         "nn-rvnd": "NN + RVND", "gils": "GILS-RVND"}
HEUR = ["nn", "nn-rvnd", "gils"]
INK, INK2, GRID, SURFACE = "#0b0b0b", "#52514e", "#e4e3df", "#fcfcfb"

plt.rcParams.update({
    "figure.facecolor": SURFACE, "axes.facecolor": SURFACE, "savefig.facecolor": SURFACE,
    "axes.edgecolor": GRID, "axes.labelcolor": INK2, "xtick.color": INK2, "ytick.color": INK2,
    "axes.grid": True, "grid.color": GRID, "grid.linewidth": 0.8,
    "axes.spines.top": False, "axes.spines.right": False,
    "font.size": 11, "axes.titlesize": 13, "axes.titleweight": "bold", "axes.titlecolor": INK,
    "legend.frameon": False, "lines.linewidth": 2, "lines.markersize": 7,
})


# --------------------------------------------------------------------------- utilitários
def read(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def save(fig, out):
    fig.tight_layout()
    fig.savefig(out, dpi=160)
    plt.close(fig)
    print("  ", out)


def md_table(header, rows):
    s = "| " + " | ".join(header) + " |\n|" + "|".join("---:" for _ in header) + "|\n"
    return s + "".join("| " + " | ".join(str(x) for x in r) + " |\n" for r in rows)


def fmt_ms(x):
    return f"{x:.3f}" if x < 1 else f"{x:.2f}" if x < 100 else f"{x:.0f}"


def fmt_time(ms):
    """Tempo legível para os achados (ms ou s)."""
    return f"{ms:.1f} ms" if ms < 1000 else f"{ms / 1000:.1f} s"


def slope_loglog(xs, ys):
    return float(np.polyfit(np.log(xs), np.log(ys), 1)[0])


# --------------------------------------------------------------------------- exp. 1
def exact(res, figs, md, finds):
    rows = read(res / "exact.csv")
    t = defaultdict(list)
    mem = defaultdict(list)
    for r in rows:
        key = (r["algo"], int(r["m"]))
        t[key].append(float(r["time_ms"]))
        if r.get("peak_rss_mb") not in (None, "", "-1"):
            mem[int(r["m"])].append(float(r["peak_rss_mb"]))
    med = {k: median(v) for k, v in t.items()}
    ms_all = sorted({m for (_, m) in t})

    fig, ax = plt.subplots(figsize=(8, 5))
    factor = {}
    for algo in ("brute", "dp"):
        ms_ = sorted(m for (a, m) in t if a == algo)
        ys = [med[(algo, m)] for m in ms_]
        ax.plot(ms_, ys, marker=MARKER[algo], color=COLOR[algo], label=LABEL[algo])
        theo = (lambda m: math.factorial(m) * m) if algo == "brute" else (lambda m: (2 ** m) * m * m)
        c = ys[-1] / theo(ms_[-1])
        xs = np.arange(ms_[0], ms_[-1] + 1)
        ax.plot(xs, [c * theo(int(x)) for x in xs], ls="--", lw=1.2, color=COLOR[algo], alpha=0.6,
                label=("c·m!·m" if algo == "brute" else "c·2ᵐ·m²") + " (teórico)")
        pts = [(m, y) for m, y in zip(ms_, ys) if y > 1]  # só pontos > 1 ms (menos ruído)
        if len(pts) >= 3:
            factor[algo] = 2 ** np.polyfit([p[0] for p in pts], [math.log2(p[1]) for p in pts], 1)[0]
    ax.set_yscale("log")
    ax.xaxis.set_major_locator(mticker.MaxNLocator(integer=True))
    ax.set_xlabel("m (nº de clientes)")
    ax.set_ylabel("tempo (ms, escala log)")
    ax.set_title("Exatos: força bruta O(m!·m) vs DP O(2ᵐ·m²)")
    ax.legend()
    save(fig, figs / "fig1_exatos_tempo.png")

    # Memória: pico medido vs tamanho teórico das tabelas (dp: 8 B, parent: 1 B por célula).
    table_mb = {m: (2 ** m) * m * 9 / 2 ** 20 for m in ms_all}
    if mem:
        # O processo já ocupa alguns MB antes da DP (código, bibliotecas). Plotamos o
        # ACRÉSCIMO sobre essa base, que é o que a DP de fato alocou.
        base = min(min(v) for v in mem.values())
        mm = [m for m in sorted(mem) if max(mem[m]) - base > 0.5]
        if len(mm) >= 2:
            fig, ax = plt.subplots(figsize=(8, 5))
            ax.plot(mm, [max(mem[m]) - base for m in mm], marker="o", color=COLOR["dp"],
                    label=f"pico medido − base do processo ({base:.1f} MB)")
            ax.plot(mm, [table_mb[m] for m in mm], ls="--", lw=1.2, color=COLOR["dp"], alpha=0.6,
                    label="tabelas dp + parent: 2ᵐ·m·9 B (teórico)")
            ax.set_yscale("log")
            ax.xaxis.set_major_locator(mticker.MaxNLocator(integer=True))
            ax.set_xlabel("m (nº de clientes)")
            ax.set_ylabel("memória (MB, escala log)")
            ax.set_title("Memória da DP: dobra a cada cliente")
            ax.legend()
            save(fig, figs / "fig7_dp_memoria.png")

    md.append("## 1. Algoritmos exatos (mediana entre 3 instâncias)\n")
    table = []
    for m in ms_all:
        b, d = med.get(("brute", m)), med[("dp", m)]
        bp, dp_ = med.get(("brute", m - 1)), med.get(("dp", m - 1))
        line = [m, fmt_ms(b) if b is not None else "—",
                f"{b / bp:.1f}×" if b and bp and bp > 0.05 else "—",
                fmt_ms(d), f"{d / dp_:.2f}×" if dp_ and dp_ > 0.05 else "—",
                f"{table_mb[m]:.1f}"]
        if mem:
            line.append(f"{max(mem[m]):.1f}" if m in mem else "—")
        table.append(line)
    header = ["m", "Força bruta (ms)", "T(m)/T(m−1)", "DP (ms)", "T(m)/T(m−1)", "Tabelas DP (MB, teórico)"]
    if mem:
        header.append("Pico medido (MB)")
    md.append(md_table(header, table))
    md.append("\nRazão esperada: força bruta ≈ m; DP ≈ 2·(m/(m−1))² → 2. O pico medido inclui alguns MB "
              "fixos do processo (código, bibliotecas).\n\n")

    mb = max(m for (a, m) in t if a == "brute")
    mx = ms_all[-1]
    finds.append(f"**Exatos.** Com {mb} clientes, força bruta {fmt_time(med[('brute', mb)])} vs DP "
                 f"{fmt_time(med[('dp', mb)])} ({med[('brute', mb)] / med[('dp', mb)]:.0f}× mais rápida). "
                 + ("Fator por cliente ajustado: "
                    + ", ".join(f"{ {'brute': 'força bruta', 'dp': 'DP'}[a]} {factor[a]:.2f}×" for a in ("brute", "dp") if a in factor)
                    + " (teoria: ≈ m e ≈ 2). " if factor else "")
                 + f"DP com {mx} clientes: {fmt_time(med[('dp', mx)])}"
                 + (f", pico de memória {max(mem[mx]):.0f} MB medido ({table_mb[mx]:.0f} MB só nas tabelas)."
                    if mx in mem else f", {table_mb[mx]:.0f} MB nas tabelas."))


# --------------------------------------------------------------------------- exp. 2
def quality(res, figs, md, finds):
    rows = read(res / "quality.csv")
    g, tm = defaultdict(list), defaultdict(list)
    for r in rows:
        g[(r["algo"], int(r["m"]))].append(float(r["gap_pct"]))
        tm[(r["algo"], int(r["m"]))].append(float(r["time_ms"]))
    ms_ = sorted({int(r["m"]) for r in rows})

    fig, ax = plt.subplots(figsize=(8, 5))
    for a in HEUR:
        ys = [mean(g[(a, m)]) for m in ms_]
        ax.plot(ms_, ys, marker=MARKER[a], color=COLOR[a], label=LABEL[a])
        ax.annotate(f"{ys[-1]:.1f}%", (ms_[-1], ys[-1]), xytext=(6, 0), textcoords="offset points",
                    va="center", color=INK2, fontsize=9)
    ax.set_xlabel("m (nº de clientes)")
    ax.set_ylabel("gap médio em relação ao ótimo (%)")
    ax.set_title("Qualidade das heurísticas vs ótimo exato (DP)")
    ax.set_ylim(bottom=0)
    ax.legend()
    save(fig, figs / "fig2_qualidade_gap.png")

    hits = sum(1 for r in rows if r["algo"] == "gils" and abs(float(r["gap_pct"])) < 1e-12)
    tot = sum(1 for r in rows if r["algo"] == "gils")
    fig, ax = plt.subplots(figsize=(8, 5))
    for a in ("dp", "gils"):
        ax.plot(ms_, [median(tm[(a, m)]) for m in ms_], marker=MARKER[a], color=COLOR[a], label=LABEL[a])
    ax.set_yscale("log")
    ax.set_xlabel("m (nº de clientes)")
    ax.set_ylabel("tempo (ms, escala log)")
    ax.set_title("Exato vs heurística: onde a DP deixa de compensar")
    ax.text(0.98, 0.04, f"GILS-RVND encontrou o ótimo em {hits}/{tot} instâncias",
            transform=ax.transAxes, ha="right", color=INK2, fontsize=10)
    ax.legend()
    save(fig, figs / "fig3_dp_vs_gils.png")

    md.append("## 2. Heurísticas vs ótimo (10 instâncias por tamanho)\n")
    table = []
    for m in ms_:
        line = [m, fmt_ms(median(tm[("dp", m)]))]
        for a in HEUR:
            gaps = g[(a, m)]
            line += [f"{mean(gaps):.2f}% / {max(gaps):.2f}%", f"{sum(abs(x) < 1e-12 for x in gaps)}/{len(gaps)}"]
        line.append(fmt_ms(median(tm[("gils", m)])))
        table.append(line)
    md.append(md_table(["m", "DP (ms)", "NN gap méd/máx", "NN ótimos", "NN+RVND gap méd/máx", "NN+RVND ótimos",
                        "GILS gap méd/máx", "GILS ótimos", "GILS (ms)"], table))
    md.append("\n")

    # Ponto de virada: menor m em que a GILS é mais rápida que a DP.
    cross = next((m for m in ms_ if median(tm[("gils", m)]) < median(tm[("dp", m)])), None)
    nn_means = [mean(g[("nn", m)]) for m in ms_]
    nr_means = [mean(g[("nn-rvnd", m)]) for m in ms_]
    big = ms_[-1]
    finds.append(f"**Qualidade vs ótimo** ({tot} instâncias, m = {ms_[0]}..{ms_[-1]}). NN: gap médio "
                 f"{min(nn_means):.1f}–{max(nn_means):.1f}% (pior caso {max(max(g[('nn', m)]) for m in ms_):.0f}%); "
                 f"NN+RVND: {min(nr_means):.2f}–{max(nr_means):.2f}%; GILS-RVND: ótimo em {hits}/{tot}.")
    finds.append(f"**Exato × heurística.** A GILS passa a ser mais rápida que a DP a partir de m = {cross}. "
                 f"Com m = {big}: DP {fmt_time(median(tm[('dp', big)]))} vs GILS "
                 f"{fmt_time(median(tm[('gils', big)]))} "
                 f"({median(tm[('dp', big)]) / median(tm[('gils', big)]):.0f}× menos), mesma resposta.")


# --------------------------------------------------------------------------- exp. 3
def scale(res, figs, md, finds):
    rows = read(res / "scale.csv")
    best = {}
    for r in rows:
        k = (int(r["n"]), int(r["instance"]))
        best[k] = min(best.get(k, math.inf), int(r["cost"]))
    gap_best, gap_lb, tm = defaultdict(list), defaultdict(list), defaultdict(list)
    for r in rows:
        k = (int(r["n"]), int(r["instance"]))
        c, lb = int(r["cost"]), int(r["lb"])
        gap_best[(r["algo"], k[0])].append(100 * (c - best[k]) / best[k])
        gap_lb[(r["algo"], k[0])].append(100 * (c - lb) / lb)
        tm[(r["algo"], k[0])].append(float(r["time_ms"]))
    ns = sorted({int(r["n"]) for r in rows})
    medt = {(a, n): median(tm[(a, n)]) for a in HEUR for n in ns}

    # Expoentes: NN e NN+RVND num ajuste só; GILS em dois regimes, porque
    # I_ILS = min(n, 100) cresce com n até 100 e fica constante depois.
    slopes = {a: slope_loglog(ns, [medt[(a, n)] for n in ns]) for a in ("nn", "nn-rvnd")}
    lo = [n for n in ns if n <= 100]
    hi = [n for n in ns if n >= 100]
    g_lo = slope_loglog(lo, [medt[("gils", n)] for n in lo]) if len(lo) >= 2 else None
    g_hi = slope_loglog(hi, [medt[("gils", n)] for n in hi]) if len(hi) >= 2 else None

    fig, ax = plt.subplots(figsize=(8, 5))
    for a in HEUR:
        ys = [medt[(a, n)] for n in ns]
        ax.plot(ns, ys, marker=MARKER[a], color=COLOR[a], label=LABEL[a])
        if a in slopes:
            ax.annotate(f"~n^{slopes[a]:.1f}", (ns[-1], ys[-1]), xytext=(6, 0), textcoords="offset points",
                        va="center", color=INK2, fontsize=9)
    if g_lo is not None and g_hi is not None:
        ax.axvline(100, color=INK2, lw=0.8, ls=":")
        ax.text(100, 0.03, " I_ILS = min(n, 100) para de crescer", color=INK2, fontsize=9,
                transform=ax.get_xaxis_transform())
        y_lo = math.sqrt(medt[("gils", lo[0])] * medt[("gils", 100)])
        y_hi = math.sqrt(medt[("gils", 100)] * medt[("gils", hi[-1])])
        ax.annotate(f"~n^{g_lo:.1f}", (math.sqrt(lo[0] * 100), y_lo), xytext=(-10, 10), textcoords="offset points",
                    color=COLOR["gils"], fontsize=9, ha="right")
        ax.annotate(f"~n^{g_hi:.1f}", (math.sqrt(100 * hi[-1]), y_hi), xytext=(-10, 10), textcoords="offset points",
                    color=COLOR["gils"], fontsize=9, ha="right")
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xticks(ns)
    ax.set_xticklabels([str(n) for n in ns])
    ax.xaxis.set_minor_formatter(mticker.NullFormatter())
    ax.set_xlabel("n (vértices, escala log)")
    ax.set_ylabel("tempo (ms, escala log)")
    ax.set_title("Tempo das heurísticas (inclinação = expoente empírico)")
    ax.legend(loc="upper left")
    save(fig, figs / "fig4_escala_tempo.png")

    fig, ax = plt.subplots(figsize=(8, 5))
    width, x = 0.26, np.arange(len(ns))
    labels = {"nn": LABEL["nn"], "nn-rvnd": LABEL["nn-rvnd"], "gils": "GILS-RVND (variação entre sementes)"}
    for i, a in enumerate(HEUR):
        ys = [mean(gap_best[(a, n)]) for n in ns]
        bars = ax.bar(x + (i - 1) * width, ys, width - 0.03, color=COLOR[a], label=labels[a])
        for b, y in zip(bars, ys):
            ax.annotate(f"{y:.1f}", (b.get_x() + b.get_width() / 2, y), xytext=(0, 3), textcoords="offset points",
                        ha="center", fontsize=8, color=INK2)
    ax.set_xticks(x)
    ax.set_xticklabels([f"n = {n}" for n in ns])
    ax.set_ylabel("gap médio vs melhor conhecida (%)")
    ax.set_title("Qualidade em instâncias grandes (sem ótimo conhecido)")
    ax.grid(axis="x", visible=False)
    ax.legend()
    save(fig, figs / "fig5_escala_gap.png")

    md.append("## 3. Escalabilidade das heurísticas (5 instâncias; GILS com várias sementes)\n")
    table = []
    for n in ns:
        line = [n]
        for a in HEUR:
            line += [f"{mean(gap_best[(a, n)]):.2f}%", f"{mean(gap_lb[(a, n)]):.1f}%", fmt_ms(medt[(a, n)])]
        table.append(line)
    md.append(md_table(["n", "NN gap best", "NN gap LB", "NN (ms)", "NN+RVND gap best", "NN+RVND gap LB",
                        "NN+RVND (ms)", "GILS gap best", "GILS gap LB", "GILS (ms)"], table))
    md.append(
        "\n*gap best* = distância à melhor solução encontrada por qualquer método nesta instância. Essa "
        "referência quase sempre vem da própria GILS, então para ela o número mede a **variação entre "
        "sementes**, não a distância ao ótimo. *gap LB* = distância ao limitante MST (o gap real ao ótimo é "
        "menor que isso).\n\n"
        f"Expoentes empíricos de tempo: NN n^{slopes['nn']:.2f}, NN+RVND n^{slopes['nn-rvnd']:.2f}"
        + (f", GILS n^{g_lo:.2f} para n ≤ 100 e n^{g_hi:.2f} para n ≥ 100" if g_lo is not None and g_hi is not None
           else "")
        + ".\n\n")

    mid = [n for n in ns if n >= 50] or ns
    nn_g = [mean(gap_best[("nn", n)]) for n in mid]
    nr_g = [mean(gap_best[("nn-rvnd", n)]) for n in mid]
    gl_g = [mean(gap_best[("gils", n)]) for n in ns]
    gl_lb = [mean(gap_lb[("gils", n)]) for n in ns]
    finds.append(f"**Escala** (n = {mid[0]}..{mid[-1]}). Acima da melhor solução conhecida: NN "
                 f"{min(nn_g):.0f}–{max(nn_g):.0f}%, NN+RVND {min(nr_g):.1f}–{max(nr_g):.1f}%; a GILS varia no máximo "
                 f"{max(gl_g):.2f}% entre sementes. Limitante MST fica {min(gl_lb):.0f}–{max(gl_lb):.0f}% abaixo da GILS.")
    finds.append(f"**Tempo em escala.** NN ~n^{slopes['nn']:.1f}, NN+RVND ~n^{slopes['nn-rvnd']:.1f}"
                 + (f"; GILS ~n^{g_lo:.1f} até n = 100 e ~n^{g_hi:.1f} depois (I_ILS = min(n, 100) para de crescer)"
                    if g_lo is not None and g_hi is not None else "")
                 + f". GILS com n = {ns[-1]}: {fmt_time(medt[('gils', ns[-1])])}.")
    return best


# --------------------------------------------------------------------------- exp. 4
def tradeoff(res, figs, md, finds, best_scale):
    rows = read(res / "tradeoff.csv")
    best = {}
    for r in rows:
        k = (int(r["n"]), int(r["instance"]))
        best[k] = min(best.get(k, math.inf), int(r["cost"]), best_scale.get(k, math.inf))
    gaps, times = defaultdict(list), defaultdict(list)
    for r in rows:
        k = (int(r["n"]), int(r["instance"]))
        gaps[int(r["iils"])].append(100 * (int(r["cost"]) - best[k]) / best[k])
        times[int(r["iils"])].append(float(r["time_ms"]) / 1000)
    its = sorted(gaps)
    n = int(rows[0]["n"])
    n_inst = len({r["instance"] for r in rows})
    n_seed = len({r.get("seed", "1") for r in rows})
    xs = [mean(times[i]) for i in its]
    ys = [mean(gaps[i]) for i in its]
    sd = [pstdev(gaps[i]) for i in its]

    # Barras = faixa entre a melhor e a pior execução (o desvio-padrão simétrico
    # desenharia gaps negativos, que não existem).
    lo_err = [y - min(gaps[i]) for i, y in zip(its, ys)]
    hi_err = [max(gaps[i]) - y for i, y in zip(its, ys)]
    fig, ax = plt.subplots(figsize=(8, 5))
    ax.errorbar(xs, ys, yerr=[lo_err, hi_err], marker="o", color=COLOR["gils"], capsize=4, elinewidth=1.2,
                label=f"média; barra = melhor e pior de {n_inst * n_seed} execuções "
                      f"({n_inst} instâncias × {n_seed} sementes)")
    for i, xv, yv in zip(its, xs, ys):
        ax.annotate(f"I_ILS={i}", (xv, yv), xytext=(6, 6), textcoords="offset points", fontsize=9, color=INK2)
    ax.set_xscale("log")
    ax.set_xlim(min(xs) * 0.8, max(xs) * 1.8)
    ax.set_xlabel("tempo médio por execução (s, escala log)")
    ax.set_ylabel("gap vs melhor conhecida (%)")
    ax.set_title(f"Exatidão × tempo da GILS-RVND (n = {n})")
    ax.set_ylim(bottom=0)
    ax.legend(fontsize=9)
    save(fig, figs / "fig6_tradeoff.png")

    md.append(f"## 4. Trade-off exatidão × tempo — GILS-RVND, n = {n}, I_max = 10, "
              f"{n_inst} instâncias × {n_seed} sementes\n")
    md.append(md_table(["I_ILS", "tempo médio (s)", "gap médio (%)", "desvio-padrão (pp)", "gap máximo (%)"],
                       [[i, f"{x:.2f}", f"{y:.2f}", f"{s:.2f}", f"{max(gaps[i]):.2f}"]
                        for i, x, y, s in zip(its, xs, ys, sd)]))
    md.append("\n")

    # Menor I_ILS com gap médio ≤ 0,1%: "a partir daqui quase não compensa insistir".
    knee = next((i for i, y in zip(its, ys) if y <= 0.1), its[-1])
    ki = its.index(knee)
    pts = [(its[0], 0)] + ([(knee, ki)] if ki not in (0, len(its) - 1) else []) + [(its[-1], len(its) - 1)]
    desc = "; ".join(f"I_ILS = {i}: {xs[j]:.2f} s e {ys[j]:.2f}% (±{sd[j]:.2f})" for i, j in pts)
    finds.append(f"**Trade-off** (n = {n}, {n_inst}×{n_seed} execuções por ponto, gap vs melhor conhecida). {desc}. "
                 f"Primeiro I_ILS com gap médio ≤ 0,1%: {knee}; daí até {its[-1]} o tempo cresce "
                 f"{xs[-1] / xs[ki]:.1f}× por {ys[ki] - ys[-1]:.2f} ponto percentual.")


# --------------------------------------------------------------------------- exp. 5
def perturb(res, md, finds):
    path = res / "perturb.csv"
    if not path.exists():
        return
    rows = read(path)
    agg = defaultdict(lambda: [0, 0, 0])
    for r in rows:
        a = agg[int(r["n"])]
        a[0] += int(r["trials"])
        a[1] += int(r["same"])
        a[2] += int(r["improved"])
    ns = sorted(agg)
    pct = {n: 100 * agg[n][1] / agg[n][0] for n in ns}
    md.append("## 5. Perturbação: double-bridge + RVND volta ao mesmo ótimo local?\n")
    md.append(md_table(["n", "tentativas", "voltou à mesma rota", "melhorou"],
                       [[n, agg[n][0], f"{pct[n]:.1f}%", f"{100 * agg[n][2] / agg[n][0]:.1f}%"] for n in ns]))
    md.append("\nCom menos de 20 clientes, max(1, m/10) = 1: os dois segmentos têm tamanho 1 e a perturbação "
              "vira uma troca que a vizinhança swap desfaz.\n\n")
    finds.append("**Perturbação.** Double-bridge + RVND volta à mesma rota em "
                 + ", ".join(f"{pct[n]:.0f}% (n = {n})" for n in ns)
                 + " — explica o retorno decrescente de aumentar I_ILS.")


# ---------------------------------------------------------------------------
def main():
    res = Path(sys.argv[1] if len(sys.argv) > 1 else "results")
    figs = res / "figures"
    figs.mkdir(parents=True, exist_ok=True)
    body, finds = [], []
    exact(res, figs, body, finds)
    quality(res, figs, body, finds)
    best = scale(res, figs, body, finds)
    tradeoff(res, figs, body, finds, best)
    perturb(res, body, finds)

    head = ["# Resultados experimentais — Minimum Latency Problem\n"]
    env = res / "environment.txt"
    if env.exists():
        head.append("```\n" + env.read_text().strip() + "\n```\n")
    head.append("## Principais achados\n")
    head.append("\n".join(f"- {f}" for f in finds) + "\n")
    (res / "summary.md").write_text("\n".join(head + body))
    print("  ", res / "summary.md")


if __name__ == "__main__":
    main()
