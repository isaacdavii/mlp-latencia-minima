# Resultados experimentais — Minimum Latency Problem

```
data: 2026-11-24T11:44:01-03:00
so: Linux 7.0.0-44-generic x86_64
cpu: 13th Gen Intel(R) Core(TM) i5-13420H
nucleos: 12
compilador: c++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
flags: -O2 -std=c++17 (CMAKE_BUILD_TYPE=Release)
```

## Principais achados

- **Exatos.** Com 11 clientes, força bruta 346.9 ms vs DP 0.6 ms (581× mais rápida). Fator por cliente ajustado: força bruta 10.26×, DP 2.23× (teoria: ≈ m e ≈ 2). DP com 20 clientes: 843.6 ms, pico de memória 203 MB medido (180 MB só nas tabelas).
- **Qualidade vs ótimo** (60 instâncias, m = 8..18). NN: gap médio 3.9–6.0% (pior caso 20%); NN+RVND: 0.00–0.69%; GILS-RVND: ótimo em 60/60.
- **Exato × heurística.** A GILS passa a ser mais rápida que a DP a partir de m = 12. Com m = 18: DP 182.9 ms vs GILS 3.5 ms (53× menos), mesma resposta.
- **Escala** (n = 50..200). Acima da melhor solução conhecida: NN 17–20%, NN+RVND 1.7–5.2%; a GILS varia no máximo 0.15% entre sementes. Limitante MST fica 37–44% abaixo da GILS.
- **Tempo em escala.** NN ~n^1.8, NN+RVND ~n^2.8; GILS ~n^4.0 até n = 100 e ~n^3.2 depois (I_ILS = min(n, 100) para de crescer). GILS com n = 200: 23.6 s.
- **Trade-off** (n = 100, 5×3 execuções por ponto, gap vs melhor conhecida). I_ILS = 1: 0.05 s e 1.14% (±0.55); I_ILS = 25: 0.78 s e 0.06% (±0.13); I_ILS = 100: 2.44 s e 0.00% (±0.01). Primeiro I_ILS com gap médio ≤ 0,1%: 25; daí até 100 o tempo cresce 3.2× por 0.06 ponto percentual.
- **Perturbação.** Double-bridge + RVND volta à mesma rota em 97% (n = 10), 95% (n = 20), 95% (n = 30), 80% (n = 60), 68% (n = 100) — explica o retorno decrescente de aumentar I_ILS.

## 1. Algoritmos exatos (mediana entre 3 instâncias)

| m | Força bruta (ms) | T(m)/T(m−1) | DP (ms) | T(m)/T(m−1) | Tabelas DP (MB, teórico) | Pico medido (MB) |
|---:|---:|---:|---:|---:|---:|---:|
| 3 | 0.000 | — | 0.000 | — | 0.0 | 3.9 |
| 4 | 0.000 | — | 0.000 | — | 0.0 | 3.9 |
| 5 | 0.001 | — | 0.001 | — | 0.0 | 3.9 |
| 6 | 0.005 | — | 0.004 | — | 0.0 | 3.9 |
| 7 | 0.035 | — | 0.013 | — | 0.0 | 3.9 |
| 8 | 0.313 | — | 0.039 | — | 0.0 | 3.9 |
| 9 | 3.29 | 10.5× | 0.132 | — | 0.0 | 3.9 |
| 10 | 29.79 | 9.0× | 0.243 | 1.84× | 0.1 | 3.9 |
| 11 | 347 | 11.6× | 0.597 | 2.46× | 0.2 | 4.0 |
| 12 | — | — | 1.40 | 2.34× | 0.4 | 4.4 |
| 13 | — | — | 3.15 | 2.25× | 0.9 | 5.0 |
| 14 | — | — | 7.01 | 2.22× | 2.0 | 6.4 |
| 15 | — | — | 15.51 | 2.21× | 4.2 | 9.5 |
| 16 | — | — | 34.07 | 2.20× | 9.0 | 16.1 |
| 17 | — | — | 75.87 | 2.23× | 19.1 | 29.8 |
| 18 | — | — | 175 | 2.31× | 40.5 | 59.0 |
| 19 | — | — | 379 | 2.16× | 85.5 | 99.0 |
| 20 | — | — | 844 | 2.23× | 180.0 | 203.0 |


Razão esperada: força bruta ≈ m; DP ≈ 2·(m/(m−1))² → 2. O pico medido inclui alguns MB fixos do processo (código, bibliotecas).


## 2. Heurísticas vs ótimo (10 instâncias por tamanho)

| m | DP (ms) | NN gap méd/máx | NN ótimos | NN+RVND gap méd/máx | NN+RVND ótimos | GILS gap méd/máx | GILS ótimos | GILS (ms) |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 8 | 0.067 | 4.59% / 14.75% | 6/10 | 0.44% / 2.91% | 8/10 | 0.00% / 0.00% | 10/10 | 0.534 |
| 10 | 0.335 | 5.19% / 12.42% | 2/10 | 0.00% / 0.00% | 10/10 | 0.00% / 0.00% | 10/10 | 0.936 |
| 12 | 1.43 | 5.33% / 13.73% | 2/10 | 0.69% / 2.97% | 7/10 | 0.00% / 0.00% | 10/10 | 0.997 |
| 14 | 7.46 | 6.03% / 19.95% | 3/10 | 0.06% / 0.60% | 9/10 | 0.00% / 0.00% | 10/10 | 2.21 |
| 16 | 35.63 | 5.73% / 17.09% | 2/10 | 0.21% / 2.06% | 9/10 | 0.00% / 0.00% | 10/10 | 2.23 |
| 18 | 183 | 3.89% / 7.61% | 2/10 | 0.49% / 3.60% | 7/10 | 0.00% / 0.00% | 10/10 | 3.48 |



## 3. Escalabilidade das heurísticas (5 instâncias; GILS com várias sementes)

| n | NN gap best | NN gap LB | NN (ms) | NN+RVND gap best | NN+RVND gap LB | NN+RVND (ms) | GILS gap best | GILS gap LB | GILS (ms) |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 20 | 4.71% | 49.4% | 0.001 | 0.00% | 42.8% | 0.024 | 0.00% | 42.8% | 4.11 |
| 50 | 20.50% | 64.7% | 0.004 | 1.68% | 39.4% | 0.340 | 0.00% | 37.2% | 149 |
| 100 | 17.98% | 69.6% | 0.016 | 5.24% | 51.3% | 2.46 | 0.00% | 43.7% | 2496 |
| 200 | 16.83% | 60.9% | 0.063 | 3.57% | 42.6% | 13.73 | 0.15% | 37.9% | 23642 |


*gap best* = distância à melhor solução encontrada por qualquer método nesta instância. Essa referência quase sempre vem da própria GILS, então para ela o número mede a **variação entre sementes**, não a distância ao ótimo. *gap LB* = distância ao limitante MST (o gap real ao ótimo é menor que isso).

Expoentes empíricos de tempo: NN n^1.82, NN+RVND n^2.77, GILS n^3.98 para n ≤ 100 e n^3.24 para n ≥ 100.


## 4. Trade-off exatidão × tempo — GILS-RVND, n = 100, I_max = 10, 5 instâncias × 3 sementes

| I_ILS | tempo médio (s) | gap médio (%) | desvio-padrão (pp) | gap máximo (%) |
|---:|---:|---:|---:|---:|
| 1 | 0.05 | 1.14 | 0.55 | 2.65 |
| 2 | 0.07 | 1.29 | 0.69 | 2.81 |
| 5 | 0.16 | 0.76 | 0.67 | 2.60 |
| 10 | 0.31 | 0.29 | 0.52 | 1.99 |
| 25 | 0.78 | 0.06 | 0.13 | 0.53 |
| 50 | 1.34 | 0.00 | 0.00 | 0.00 |
| 100 | 2.44 | 0.00 | 0.01 | 0.04 |



## 5. Perturbação: double-bridge + RVND volta ao mesmo ótimo local?

| n | tentativas | voltou à mesma rota | melhorou |
|---:|---:|---:|---:|
| 10 | 500 | 96.6% | 0.4% |
| 20 | 500 | 94.8% | 0.0% |
| 30 | 500 | 95.4% | 0.6% |
| 60 | 500 | 80.0% | 3.0% |
| 100 | 500 | 68.2% | 3.6% |


Com menos de 20 clientes, max(1, m/10) = 1: os dois segmentos têm tamanho 1 e a perturbação vira uma troca que a vizinhança swap desfaz.

