// =============================================================================
// solution.hpp — Rota, função objetivo (latência) e validação
// -----------------------------------------------------------------------------
// Definição adotada (versão "caminho", sem retorno ao depósito):
//
//   Rota π = (π_0 = 0, π_1, ..., π_m), m = n - 1 clientes.
//   Tempo de chegada:  t(π_k) = Σ_{i=1..k} d(π_{i-1}, π_i)
//   Latência total:    L(π)  = Σ_{k=1..m} t(π_k)
//
// Reformulação útil (base da DP e da avaliação O(1)): a aresta que chega na
// posição k é "paga" por todos os clientes a partir de k, isto é, m - k + 1
// vezes:
//                     L(π) = Σ_{k=1..m} (m - k + 1) · d(π_{k-1}, π_k)
// =============================================================================
#pragma once

#include <string>
#include <vector>

#include "mlp/instance.hpp"

namespace mlp {

/// Rota: route[0] == 0 (depósito), seguida de uma permutação de 1..n-1.
using Route = std::vector<int>;

/// Resultado de um algoritmo: rota e seu custo (latência total).
struct Result {
    Route route;
    Cost cost = 0;
};

/// Latência total da rota — O(n).
Cost latency(const Instance& inst, const Route& route);

/// Verifica se a rota começa no depósito e visita cada cliente exatamente uma vez.
bool is_valid_route(const Instance& inst, const Route& route);

/// Rota identidade (0, 1, 2, ..., n-1) — útil para testes.
Route identity_route(int n);

/// Representação textual "0 -> 3 -> 1 -> ...".
std::string route_to_string(const Route& route);

}  // namespace mlp
