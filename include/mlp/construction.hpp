// =============================================================================
// construction.hpp — Heurísticas construtivas
// -----------------------------------------------------------------------------
//  • Vizinho mais próximo (determinística)          — O(n²)  [baseline]
//  • Construção gulosa aleatorizada (GRASP, α)      — O(n² log n)
// =============================================================================
#pragma once

#include "mlp/instance.hpp"
#include "mlp/rng.hpp"
#include "mlp/solution.hpp"

namespace mlp {

/// Vizinho mais próximo: a partir do vértice atual, vai sempre ao cliente não
/// visitado mais próximo. Intuição para a latência: arestas curtas no início
/// são as que mais pesam (multiplicador m - k + 1).
Route nearest_neighbor(const Instance& inst);

/// Vizinho mais próximo empacotado como Result (rota + custo). Baseline heurístico.
Result solve_nn(const Instance& inst);

/// Construção GRASP: em cada passo, ordena os candidatos pela distância ao
/// último vértice e sorteia um entre os ⌈α·|CL|⌉ melhores (Lista Restrita de
/// Candidatos). α = 0 ⇒ vizinho mais próximo; α = 1 ⇒ totalmente aleatória.
Route grasp_construction(const Instance& inst, double alpha, Rng& rng);

}  // namespace mlp
