// =============================================================================
// algorithms.hpp — Registro único dos algoritmos (nome → função)
// -----------------------------------------------------------------------------
// Solver e benchmark consultam esta tabela em vez de repetir cadeias de "if".
// É também o único lugar onde ficam os limites práticos de tamanho: para
// incluir um algoritmo novo basta acrescentar uma linha em algorithms.cpp.
// =============================================================================
#pragma once

#include <string>
#include <vector>

#include "mlp/gils_rvnd.hpp"
#include "mlp/instance.hpp"
#include "mlp/solution.hpp"

namespace mlp {

struct Algorithm {
    std::string name;   ///< nome curto usado na CLI e nos CSVs ("dp", "gils", ...)
    std::string label;  ///< nome legível
    bool exact;         ///< garante o ótimo?
    int practical_max;  ///< maior nº de clientes em que roda em ~1 s (-1 = sem limite)
    /// Executa o algoritmo. Parâmetros ignorados pelos algoritmos que não os usam
    /// (a semente vale para nn-rvnd e gils; I_max e I_ILS, só para gils).
    Result (*run)(const Instance&, const GilsParams&);
};

/// Todos os algoritmos, em ordem: brute, dp, nn, nn-rvnd, gils.
const std::vector<Algorithm>& all_algorithms();

/// Busca pelo nome; nullptr se não existir.
const Algorithm* find_algorithm(const std::string& name);

/// Roda pelo nome (lança std::invalid_argument se o nome não existir).
Result run_algorithm(const std::string& name, const Instance& inst, const GilsParams& params = {});

}  // namespace mlp
