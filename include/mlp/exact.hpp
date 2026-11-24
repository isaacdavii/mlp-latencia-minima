// =============================================================================
// exact.hpp — Algoritmos exatos e limitante inferior
// -----------------------------------------------------------------------------
//  • Força bruta:   enumera as m! permutações          — O(m! · m)
//  • DP (Held–Karp adaptada à latência):               — O(2^m · m²) tempo
//                                                         O(2^m · m)  memória
//  • Limitante inferior via MST (válido também para matrizes assimétricas)
// =============================================================================
#pragma once

#include "mlp/instance.hpp"
#include "mlp/solution.hpp"

namespace mlp {

/// Limites práticos (acima disso o tempo/memória explode).
inline constexpr int kMaxBruteForceCustomers = 12;  ///< 12! ≈ 4,8·10^8 permutações
inline constexpr int kMaxDpCustomers = 22;          ///< 2^22·22·8 B ≈ 740 MB

/// Força bruta: testa todas as permutações dos clientes. Baseline exato.
Result solve_brute_force(const Instance& inst);

/// Programação dinâmica sobre subconjuntos.
///   dp[S][v] = menor latência parcial para visitar exatamente o conjunto S de
///              clientes, terminando em v ∈ S.
///   Transição (|S| = k, u ∉ S):
///              dp[S ∪ {u}][u] = min( dp[S][v] + (m - k) · d(v, u) )
///   Funciona porque, pela reformulação L = Σ (m-k+1)·d(π_{k-1}, π_k), o custo
///   de uma aresta só depende da POSIÇÃO em que ela entra — e a posição é |S|.
Result solve_dp(const Instance& inst);

/// Limitante inferior: se e_(1) ≤ ... ≤ e_(m) são as arestas da MST,
///   LB = Σ_{k=1..m} (m - k + 1) · e_(k)  ≤  OPT.
Cost mst_lower_bound(const Instance& inst);

}  // namespace mlp
