// =============================================================================
// algorithms.cpp — Tabela de algoritmos
// =============================================================================
#include "mlp/algorithms.hpp"

#include <stdexcept>

#include "mlp/construction.hpp"
#include "mlp/exact.hpp"
#include "mlp/local_search.hpp"

namespace mlp {

const std::vector<Algorithm>& all_algorithms() {
    // practical_max: medido com -O2 — força bruta com 11 clientes ≈ 0,35 s;
    // DP com 20 clientes ≈ 0,9 s e ~190 MB.
    static const std::vector<Algorithm> table = {
        {"brute", "Força bruta", true, 11,
         [](const Instance& i, const GilsParams&) { return solve_brute_force(i); }},
        {"dp", "DP sobre subconjuntos", true, 20,
         [](const Instance& i, const GilsParams&) { return solve_dp(i); }},
        {"nn", "Vizinho mais próximo", false, -1,
         [](const Instance& i, const GilsParams&) { return solve_nn(i); }},
        {"nn-rvnd", "NN + RVND", false, -1,
         [](const Instance& i, const GilsParams& p) { return nn_rvnd(i, p.seed); }},
        {"gils", "GILS-RVND", false, -1,
         [](const Instance& i, const GilsParams& p) { return gils_rvnd(i, p); }},
    };
    return table;
}

const Algorithm* find_algorithm(const std::string& name) {
    for (const auto& a : all_algorithms())
        if (a.name == name) return &a;
    return nullptr;
}

Result run_algorithm(const std::string& name, const Instance& inst, const GilsParams& params) {
    const Algorithm* a = find_algorithm(name);
    if (!a) throw std::invalid_argument("algoritmo desconhecido: " + name);
    return a->run(inst, params);
}

}  // namespace mlp
