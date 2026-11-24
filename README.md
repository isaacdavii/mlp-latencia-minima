# Minimum Latency Problem (Traveling Repairman Problem)

Trabalho Prático — Projeto e Análise de Algoritmos (CTCO04) — UNIFEI, 2º/2026 — Prof. Sávio S. Dias
Tema 1.8 

Integrantes:
- **Bianca Salvador** - [GitHub](https://github.com/biancas2-zz)
- **Isaac Davi** - [GitHub](https://github.com/isaacdavii)
- **Laura Raimundi** - [GitHub](https://github.com/laurajesus)

Implementação em **C++17** de algoritmos exatos e heurísticos para o Problema da Latência Mínima, com testes de corretude, scripts de benchmark e geração automática de gráficos e do relatório de resultados.

> **Onde estão os números.** Todos os tempos, gaps e expoentes medidos ficam em [`results/summary.md`](results/summary.md), gerado pelo `scripts/plot_results.py` a partir dos CSVs. O README não repete esses números de propósito: assim, ao rodar os benchmarks de novo, nada fica desatualizado.

---

## 1. O problema

Dado um grafo completo com vértices `0` (depósito) e `1..m` (clientes) e distâncias `d(i, j)`, encontrar uma rota `π = (0, π₁, …, π_m)` que **minimize a soma dos tempos de chegada** a todos os clientes:

```
t(π_k) = Σ_{i=1..k} d(π_{i-1}, π_i)          L(π) = Σ_{k=1..m} t(π_k)
```

Diferente do TSP (que minimiza o tempo total do servidor), o MLP otimiza a **espera dos clientes**. É NP-difícil (Sahni & Gonzalez, 1976) e admite aproximação por fator constante (Blum et al., 1994; Goemans & Kleinberg, 1998).

**Reformulação central do projeto.** A aresta que entra na posição `k` atrasa todos os clientes a partir de `k`, ou seja, é paga `m − k + 1` vezes:

```
L(π) = Σ_{k=1..m} (m − k + 1) · d(π_{k−1}, π_k)
```

Essa identidade é a base da DP exata, do limitante inferior e da avaliação O(1) de movimentos.

> Convenção adotada: versão "caminho" (sem retorno ao depósito). Distâncias euclidianas arredondadas ao inteiro mais próximo (`nint`, como no TSPLIB), o que permite comparar custos com igualdade exata nos testes. Matrizes assimétricas (`EXPLICIT`) também são aceitas.

## 2. Algoritmos implementados

| Algoritmo | Nome na CLI | Tipo | Complexidade | Arquivo |
|---|---|---|---|---|
| Força bruta (todas as permutações) | `brute` | exato, baseline | O(m!·m) | `src/exact.cpp` |
| **DP sobre subconjuntos** (Held–Karp adaptada) | `dp` | exato | O(2ᵐ·m²) tempo, O(2ᵐ·m) memória | `src/exact.cpp` |
| Limitante inferior via MST | — | limitante | O(n²) | `src/exact.cpp` |
| Vizinho mais próximo | `nn` | construtivo, baseline | O(n²) | `src/construction.cpp` |
| Vizinho mais próximo + RVND | `nn-rvnd` | busca local | O(n²) por melhoria | `src/local_search.cpp` |
| **GILS-RVND** (Silva et al., 2012) | `gils` | metaheurística | ver abaixo | `src/gils_rvnd.cpp` |

Os cinco algoritmos ficam num registro único (`src/algorithms.cpp`: nome, se é exato, limite prático de tamanho e função). O solver, o benchmark e os testes usam essa tabela, então os limites de tamanho estão definidos num lugar só.

**DP.** `dp[S][v]` = menor latência parcial visitando exatamente o conjunto `S`, terminando em `v`. Pela reformulação, a próxima aresta tem peso `m − |S|`, então só a posição importa:
`dp[S ∪ {u}][u] = min_v ( dp[S][v] + (m − |S|)·d(v, u) )`.
Memória: `dp` (8 B por célula) + `parent` (1 B) = 2ᵐ·m·9 B, cerca de 180 MB com m = 20. O pico de memória do processo é medido no experimento `exact` (Fig. 7).

**Limitante MST.** A i-ésima menor aresta de qualquer árvore geradora é ≥ a i-ésima menor aresta da MST (propriedade de matroides). Com a desigualdade de rearranjo: `L(π) ≥ Σ (m − k + 1)·mst_(k)`. Para matrizes assimétricas, a MST usa `min(d(u,v), d(v,u))`, que mantém o limitante válido.

**Avaliação O(1) por concatenação de subsequências.** Para cada trecho `σ` da rota guardamos duração `D`, custo acumulado `C` e nº de vértices `W`. Concatenar é O(1):

```
t = D₁ + d(último(σ₁), primeiro(σ₂))
D = t + D₂      C = C₁ + W₂·t + C₂      W = W₁ + W₂
```

Com a tabela de todas as subsequências (O(n²), refeita só após uma melhora), qualquer vizinho de swap, 2-opt ou or-opt-1/2/3 é avaliado juntando no máximo 5 pedaços, em vez de O(n) recalculando a rota inteira.

**GILS-RVND.** `I_max` reinícios com construção GRASP (α sorteado em {0, 0.01, …, 0.25}); em cada um, um ILS com RVND como busca local e double-bridge como perturbação, parando após `I_ILS` iterações seguidas sem melhora. Padrões: `I_max = 10`, `I_ILS = min(n, 100)`.

### Complexidade da GILS-RVND

Uma chamada do RVND faz `k` melhorias. Cada melhoria custa uma varredura de vizinhança (O(n²) vizinhos × O(1)) e uma reconstrução da tabela (O(n²)). No fim, mais 5 varreduras sem melhora confirmam o ótimo local. Logo:

```
T_RVND = O((k + 5) · n²)
T_GILS = O(I_max · R · (k + 5) · n²),   R ≥ I_ILS = nº de iterações do ILS por reinício
```

`R` é pelo menos `I_ILS` (o contador zera a cada melhora) e `k` cresce com n. Isso explica os **dois regimes** da Fig. 4:
- **n ≤ 100:** `I_ILS = n` cresce junto com a instância, somando um fator n ao custo — expoente empírico maior;
- **n > 100:** `I_ILS` fica fixo em 100 e esse fator some — o expoente cai cerca de 1 (resta n² da varredura × crescimento de `k` e `R`).

Por isso o ajuste da Fig. 4 é feito separadamente em cada trecho.

## 3. Estrutura do repositório

```
include/mlp/          interfaces públicas (documentadas)
  instance.hpp        instância, gerador, leitura/escrita TSPLIB
  solution.hpp        rota, latência, validação
  exact.hpp           força bruta, DP, limitante MST
  construction.hpp    vizinho mais próximo, construção GRASP
  subsequence.hpp     estrutura Subseq e tabela de subsequências
  local_search.hpp    vizinhanças, RVND, NN+RVND, double-bridge
  gils_rvnd.hpp       metaheurística GILS-RVND
  algorithms.hpp      registro único dos algoritmos (nome → função, limites)
  rng.hpp             gerador xoshiro256** portável (reprodutibilidade)
  timer.hpp           cronômetro monotônico
src/                  implementações
apps/
  mlp_solver.cpp      CLI: resolve uma instância
  mlp_gen.cpp         gera instâncias aleatórias (TSPLIB)
  mlp_bench.cpp       experimentos → CSV
tests/
  test_framework.hpp  mini framework de testes (sem dependências)
  test_main.cpp       27 testes de corretude
scripts/
  run_benchmarks.sh   compila + testa + roda todos os experimentos + gráficos
  plot_results.py     CSV → figuras PNG + results/summary.md
instances/            instâncias de exemplo (TSPLIB)
results/              CSVs, figuras e relatório gerado (summary.md)
```

## 4. Como compilar e executar (reprodutibilidade)

Requisitos: compilador C++17 (GCC ≥ 9, Clang ≥ 10 ou MSVC 2019+), CMake ≥ 3.14; Python 3 com `matplotlib` e `numpy` só para os gráficos.

```bash
# Compilar (Release, -O2)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# Testes
ctest --test-dir build --output-on-failure      # ou: ./build/mlp_tests

# Sem CMake (Linux/macOS/MinGW)
make && make test
```

### Resolver uma instância

```bash
./build/mlp_solver --instance instances/rand_n15_s1.tsp            # roda todos os aplicáveis ao tamanho
./build/mlp_solver --instance instances/rand_n100_s1.tsp --algo gils --seed 7 --route
./build/mlp_solver --random 18 --gen-seed 3 --algo dp               # instância gerada na hora
./build/mlp_gen 50 42 instances/minha.tsp                           # gerar arquivo TSPLIB
```

Algoritmos: `brute`, `dp`, `nn`, `nn-rvnd`, `gils`, `all`. Com `all`, cada algoritmo roda se a instância couber no seu limite prático (força bruta até 11 clientes, DP até 20). O solver confere a validade da rota e a consistência do custo antes de imprimir.
Formatos aceitos: TSPLIB `EUC_2D` e `EXPLICIT / FULL_MATRIX` (o primeiro nó é o depósito). Arquivos truncados ou malformados geram erro com mensagem, nunca uma instância incompleta.

### Reproduzir todos os resultados

```bash
python3 -m venv .venv && . .venv/bin/activate && pip install -r requirements.txt
./scripts/run_benchmarks.sh           # completo (~10 min num notebook Core i5 na tomada)
./scripts/run_benchmarks.sh --quick   # versão reduzida (~15 s)
```

Gera `results/{exact,quality,scale,tradeoff,perturb}.csv`, `results/environment.txt`, `results/figures/fig1..fig7.png` e `results/summary.md`.

**Determinismo.** Instâncias são geradas por `make_random_euclidean(n, seed)` com `seed = 1, 2, …`, e toda aleatoriedade usa o gerador próprio `mlp::Rng` (xoshiro256**). Não usamos `std::uniform_int_distribution` porque seu algoritmo não é fixado pelo padrão e difere entre GCC, Clang e MSVC. Mesma semente ⇒ mesma instância e mesma solução em qualquer máquina. Verificamos isso com três rodadas completas em duas máquinas (CPUs e versões de GCC diferentes): todos os custos coincidiram linha a linha.

**Tempos variam; custos não.** No mesmo notebook (Core i5-13420H, CPU híbrida com núcleos de desempenho e de eficiência), duas rodadas deram tempos de heurística 2,5 a 3 vezes diferentes, enquanto a DP repetiu o tempo exatamente. A causa provável é o perfil de energia e o escalonamento entre os dois tipos de núcleo durante as execuções longas. Por isso tratamos tempos como **ordem de grandeza e taxa de crescimento**, não valores absolutos. Para medir, use o notebook na tomada e no modo desempenho; no Linux, `taskset -c 0 ./scripts/run_benchmarks.sh` prende a execução num único núcleo.

## 5. Testes de corretude

Cada componente é validado contra uma referência independente (`tests/test_main.cpp`, 27 testes):

| Teste | O que garante |
|---|---|
| `latencia_exemplo_manual`, `latencia_formula_reformulada` | função objetivo e a identidade `Σ(m−k+1)·d` |
| `subsequencias_*` | tabela de subsequências (direta e reversa) coincide com a avaliação direta |
| `vizinhanca_{swap,2opt,oropt}_avaliacao_O1_exata` | para **todos** os vizinhos, custo previsto em O(1) = custo real após aplicar o movimento |
| `rvnd_produz_otimo_local` | após o RVND, nenhuma das 5 vizinhanças melhora a rota |
| `dp_igual_forca_bruta` | DP = força bruta em 250 instâncias (m = 0..9); a rota reconstruída tem o custo informado |
| `limitante_mst_nao_supera_otimo` | LB ≤ ótimo; justo na instância em reta |
| `gils_nunca_abaixo_do_otimo_e_acha_otimo_pequeno` | GILS ≥ ótimo sempre; acha o ótimo em 40/40 instâncias pequenas |
| `extremo_instancia_assimetrica_*` | matriz assimétrica: DP = força bruta, LB ≤ ótimo, avaliação O(1) exata em todos os vizinhos |
| `extremo_pontos_coincidentes_custo_zero` | todas as distâncias 0: todos os algoritmos dão custo 0 |
| `extremo_tsplib_truncado_ou_malformado_lanca_excecao` | coordenadas faltando, matriz incompleta, sem DIMENSION, tipo não suportado, arquivo inexistente |
| `registro_de_algoritmos_completo_e_consistente` | todos os algoritmos do registro: rota válida, custo coerente, exatos = ótimo |
| `reprodutibilidade_mesma_semente`, `instancia_gerador_eh_deterministico`, … | determinismo, E/S TSPLIB, casos de borda (n = 1, 2, 3) |

Além disso, `mlp_bench` e `mlp_solver` abortam se qualquer algoritmo devolver rota inválida ou custo inconsistente, e o experimento `exact` confere DP = força bruta em todas as instâncias medidas.

## 6. Metodologia experimental

- **Instâncias:** euclidianas aleatórias, coordenadas inteiras uniformes em [0, 1000]², distância `nint`.
- **Métricas:** tempo de parede (`std::chrono::steady_clock`, mediana de 3 execuções quando a execução é curta); custo; *gap* = (custo − referência)/referência; pico de memória residente do processo (`getrusage`, Linux/macOS).
- **Referências de qualidade:** ótimo da DP quando m ≤ 18; para n maior, a melhor solução encontrada por qualquer método e o limitante MST.
- **Experimentos** (`mlp_bench`):
  1. `exact` — força bruta (m ≤ 11) vs DP (m ≤ 20), 3 instâncias por m; tempo e memória de pico.
  2. `quality` — NN, NN+RVND e GILS vs ótimo, m ∈ {8, …, 18}, 10 instâncias por m.
  3. `scale` — heurísticas em n ∈ {20, 50, 100, 200}, 5 instâncias; GILS com 3 sementes.
  4. `tradeoff` — GILS em n = 100 variando `I_ILS ∈ {1, 2, 5, 10, 25, 50, 100}`, 5 instâncias × 3 sementes, com desvio-padrão.
  5. `perturb` — a partir do ótimo local do NN+RVND, 100 perturbações double-bridge + RVND por instância (n ∈ {10, 20, 30, 60, 100}); mede quantas voltam exatamente à mesma rota.
- Ambiente registrado automaticamente em `results/environment.txt`.

## 7. Resultados e análise

Números em [`results/summary.md`](results/summary.md) (seção *Principais achados* no topo). Figuras:

| Figura | O que mostra |
|---|---|
| `fig1_exatos_tempo.png` | Força bruta e DP sobre as curvas teóricas m!·m e 2ᵐ·m²; fator de crescimento por cliente ajustado por regressão. |
| `fig7_dp_memoria.png` | Pico de memória medido da DP contra o tamanho teórico das tabelas: dobra a cada cliente e é o limite prático. |
| `fig2_qualidade_gap.png` | Gap médio de cada heurística em relação ao ótimo exato (m ≤ 18). |
| `fig3_dp_vs_gils.png` | Tempo da DP vs GILS nas mesmas instâncias: o ponto em que a heurística passa a compensar. |
| `fig4_escala_tempo.png` | Tempo das heurísticas até n = 200 (log-log); GILS ajustada em dois regimes (seção 2). |
| `fig5_escala_gap.png` | Gap das heurísticas vs melhor solução conhecida em instâncias grandes. |
| `fig6_tradeoff.png` | Exatidão × tempo da GILS variando `I_ILS`, média ± desvio-padrão. |

**Quando a heurística se torna necessária?** A força bruta é inviável a partir de m ≈ 12. A DP é a melhor escolha enquanto for mais rápida que a GILS (ponto de virada na Fig. 3), e continua viável até m ≈ 20–22. Cada cliente a mais dobra tempo e memória (Fig. 7). Acima disso, só a heurística é viável. Na faixa em que dá para comparar com o ótimo, a GILS encontrou o ótimo em todas as instâncias.

**Por que o ganho de insistir cai tão rápido (Fig. 6).** O gap cai bastante nas primeiras dezenas de iterações sem melhora e depois praticamente zera, com variação grande entre execuções nos valores baixos de `I_ILS` — por isso o gráfico mostra a faixa entre a melhor e a pior execução. (Numa primeira rodada com uma única semente, `I_ILS = 10` parecia suficiente; com 3 sementes, ficou claro que era sorte.) O mecanismo: O double-bridge troca dois segmentos de tamanho em [1, max(1, m/10)]. Com menos de 20 clientes, os dois segmentos têm sempre tamanho 1: a perturbação vira uma troca de dois clientes, que a própria vizinhança *swap* do RVND desfaz. O experimento `perturb` mede isso: a busca volta exatamente à mesma rota na grande maioria das tentativas em n pequeno, e ainda com frequência em n = 100 (tabela 5 do `summary.md`). Com isso, aumentar `I_ILS` repete trabalho que já foi feito, o que explica o retorno decrescente no trade-off. Segmentos maiores (≥ 2) diversificariam mais; deixamos como trabalho futuro, porque mudam todos os resultados.

**Onde o tempo vai.** Profiling com gprof (GILS, n = 60): `eval_or_opt` responde sozinho por cerca de 45% das amostras e por 3 de cada 4 avaliações de vizinhos, porque são 3 vizinhanças (blocos de 1, 2 e 3) com ~n² vizinhos cada. Como cada avaliação já é O(1), o próximo ganho viria de reduzir o número de vizinhos (listas de candidatos) ou de reconstruir só a parte da tabela de subsequências afetada por uma melhora, em vez da tabela inteira.

**Limitações.**
1. *Gap "best" da GILS mede variação entre sementes, não distância ao ótimo.* A melhor solução conhecida de cada instância grande quase sempre vem da própria GILS. A distância ao ótimo real só é garantida abaixo do gap ao limitante MST.
2. *O limitante MST é fraco* (fica dezenas de por cento abaixo das soluções), útil como garantia, não como medida de qualidade.
3. *Não implementamos a aproximação com garantia* (fator 3,59 de Goemans–Kleinberg), que depende de resolver k-MST; ela aparece como fundamentação.
4. *Instâncias apenas euclidianas aleatórias.* Uma referência independente seriam instâncias TSPLIB com valores publicados para o MLP (Silva et al., 2012). Antes de comparar, é preciso conferir se a convenção é a mesma (com ou sem retorno ao depósito; aqui é sem).

## 8. Referências

1. A. Blum, P. Chalasani, D. Coppersmith, B. Pulleyblank, P. Raghavan, M. Sudan. *The minimum latency problem*. STOC, 1994.
2. M. X. Goemans, J. Kleinberg. *An improved approximation ratio for the minimum latency problem*. Mathematical Programming 82, 1998.
3. M. M. Silva, A. Subramanian, T. Vidal, L. S. Ochi. *A simple and effective metaheuristic for the Minimum Latency Problem*. European Journal of Operational Research 221(3), 2012.
4. T. Vidal, T. G. Crainic, M. Gendreau, C. Prins. *A unified solution framework for multi-attribute vehicle routing problems*. EJOR 234(3), 2014 (concatenação de subsequências).
5. M. Held, R. M. Karp. *A dynamic programming approach to sequencing problems*. J. SIAM 10(1), 1962.
6. S. Sahni, T. Gonzalez. *P-complete approximation problems*. Journal of the ACM 23(3), 1976.
7. N. Mladenović, P. Hansen. *Variable neighborhood search*. Computers & Operations Research 24(11), 1997.
8. T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein. *Introduction to Algorithms*, 3ª ed. MIT Press, 2009.
