// =============================================================================
// gils_rvnd.hpp — Metaheurística GILS-RVND (Silva, Subramanian, Vidal & Ochi, 2012)
// -----------------------------------------------------------------------------
//   s* ← ∅
//   para iter = 1..I_max:                         (multi-start, estilo GRASP)
//       α ← sorteia de R = {0.00, 0.01, ..., 0.25}
//       s ← construção GRASP(α);   s' ← s
//       iterILS ← 0
//       enquanto iterILS < I_ILS:                 (Iterated Local Search)
//           s ← RVND(s)                           (busca local)
//           se f(s) < f(s'):  s' ← s;  iterILS ← 0
//           s ← double_bridge(s')                 (perturbação)
//           iterILS ← iterILS + 1
//       se f(s') < f(s*): s* ← s'
//   retorna s*
//
// Parâmetros padrão inspirados no artigo: I_max = 10, I_ILS = min(n, 100).
// Totalmente determinística dada a semente (ver rng.hpp).
// =============================================================================
#pragma once

#include <cstdint>

#include "mlp/instance.hpp"
#include "mlp/solution.hpp"

namespace mlp {

struct GilsParams {
    int max_iter = 10;        ///< I_max: nº de reinícios (construções)
    int max_iter_ils = -1;    ///< I_ILS: iterações sem melhora; -1 ⇒ min(n, 100)
    std::uint64_t seed = 1;   ///< semente do gerador
};

Result gils_rvnd(const Instance& inst, const GilsParams& params = {});

}  // namespace mlp
