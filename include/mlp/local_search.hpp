// =============================================================================
// local_search.hpp — Vizinhanças, RVND e perturbação
// -----------------------------------------------------------------------------
// Posições válidas para movimentos: 1..n-1 (o depósito na posição 0 é fixo).
//
// Vizinhanças (cada uma O(n²) vizinhos, avaliados em O(1) cada):
//   SWAP      troca os clientes das posições i e j
//   TWO_OPT   inverte o trecho [i..j]
//   OR_OPT_k  move o bloco de k clientes consecutivos (k = 1, 2, 3) para
//             antes da posição p
//
// Cada movimento tem duas funções públicas — eval_* (custo da rota resultante,
// via concatenação) e apply_* (modifica a rota) — para que os testes possam
// verificar eval_* == latency(apply_*(rota)) em TODOS os vizinhos.
// =============================================================================
#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "mlp/instance.hpp"
#include "mlp/rng.hpp"
#include "mlp/solution.hpp"
#include "mlp/subsequence.hpp"

namespace mlp {

enum class Neighborhood { Swap, TwoOpt, OrOpt1, OrOpt2, OrOpt3 };
inline constexpr std::array<Neighborhood, 5> kAllNeighborhoods{
    Neighborhood::Swap, Neighborhood::TwoOpt, Neighborhood::OrOpt1, Neighborhood::OrOpt2, Neighborhood::OrOpt3};

std::string to_string(Neighborhood nb);

// --- Avaliação O(1) -----------------------------------------------------------
Cost eval_swap(const Instance& inst, const SubseqTable& t, int i, int j);     ///< i < j
Cost eval_two_opt(const Instance& inst, const SubseqTable& t, int i, int j);  ///< i < j
/// Bloco [i, i+k-1] inserido antes da posição p, com p < i ou p > i+k.
/// p == n (tamanho da rota) significa "no final".
Cost eval_or_opt(const Instance& inst, const SubseqTable& t, int i, int k, int p);

// --- Aplicação ----------------------------------------------------------------
void apply_swap(Route& r, int i, int j);
void apply_two_opt(Route& r, int i, int j);
void apply_or_opt(Route& r, int i, int k, int p);

/// Procura o MELHOR vizinho (best improvement) na vizinhança. Se ele melhora a
/// rota, aplica o movimento, atualiza `cost` e retorna true.
bool best_improvement(const Instance& inst, Route& route, Cost& cost, Neighborhood nb, const SubseqTable& t);

/// RVND — Random Variable Neighborhood Descent (Mladenović & Hansen; Subramanian et al.):
///   NL ← todas as vizinhanças
///   enquanto NL ≠ ∅:
///       N ← sorteia de NL
///       se best_improvement(N) melhorou: NL ← todas  (e reconstrói a tabela)
///       senão:                          NL ← NL \ {N}
/// Ao final, a rota é ótimo local em relação a TODAS as vizinhanças.
/// Retorna o custo final.
Cost rvnd(const Instance& inst, Route& route, Rng& rng);

/// Busca local simples (heurística intermediária): vizinho mais próximo + RVND.
Result nn_rvnd(const Instance& inst, std::uint64_t seed = 1);

/// Perturbação double-bridge: troca dois segmentos disjuntos e aleatórios
/// (tamanhos em [1, max(1, m/10)]). Retira a solução da bacia de atração do
/// ótimo local sem destruí-la por completo.
Route double_bridge(const Route& route, Rng& rng);

}  // namespace mlp
