// =============================================================================
// exact.cpp — Força bruta, DP sobre subconjuntos e limitante inferior MST
// =============================================================================
#include "mlp/exact.hpp"

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace mlp {

// -----------------------------------------------------------------------------
// Força bruta
// -----------------------------------------------------------------------------
Result solve_brute_force(const Instance& inst) {
    const int m = inst.customers();
    if (m > kMaxBruteForceCustomers)
        throw std::invalid_argument("força bruta limitada a " + std::to_string(kMaxBruteForceCustomers) +
                                    " clientes");

    Route perm = identity_route(inst.n);  // (0, 1, 2, ..., m) — já ordenada
    Result best{perm, latency(inst, perm)};

    // next_permutation percorre as permutações em ordem lexicográfica a partir
    // da posição 1 (o depósito na posição 0 nunca se move).
    while (std::next_permutation(perm.begin() + 1, perm.end())) {
        const Cost c = latency(inst, perm);
        if (c < best.cost) best = {perm, c};
    }
    return best;
}

// -----------------------------------------------------------------------------
// Programação dinâmica sobre subconjuntos
// -----------------------------------------------------------------------------
// Cliente c ∈ {1..m} é representado pelo bit (c - 1) da máscara.
// Tabelas planas de tamanho 2^m · m (índice = mask * m + v) para localidade.
Result solve_dp(const Instance& inst) {
    const int m = inst.customers();
    if (m == 0) return {Route{0}, 0};
    if (m > kMaxDpCustomers)
        throw std::invalid_argument("DP limitada a " + std::to_string(kMaxDpCustomers) + " clientes");

    constexpr Cost INF = std::numeric_limits<Cost>::max() / 4;
    const std::size_t full = (std::size_t{1} << m);
    std::vector<Cost> dp(full * m, INF);
    std::vector<std::int8_t> parent(full * m, -1);  // último cliente antes de v (-1 = depósito)

    auto idx = [m](std::size_t mask, int v) { return mask * m + v; };

    // Caso base: primeiro cliente visitado. A aresta 0 -> v é paga por todos os m clientes.
    for (int v = 0; v < m; ++v) dp[idx(std::size_t{1} << v, v)] = static_cast<Cost>(m) * inst.d(0, v + 1);

    // Máscaras em ordem crescente garantem que S é processado antes de S ∪ {u}.
    for (std::size_t mask = 1; mask < full; ++mask) {
        const int k = static_cast<int>(std::bitset<64>(mask).count());  // nº de clientes já visitados (|S|)
        const Cost weight = m - k;                 // multiplicador da próxima aresta
        if (weight == 0) continue;                 // máscara completa: nada a estender

        for (int v = 0; v < m; ++v) {
            if (!(mask >> v & 1)) continue;
            const Cost base = dp[idx(mask, v)];
            if (base >= INF) continue;

            for (int u = 0; u < m; ++u) {
                if (mask >> u & 1) continue;
                const std::size_t nmask = mask | (std::size_t{1} << u);
                const Cost cand = base + weight * inst.d(v + 1, u + 1);
                if (cand < dp[idx(nmask, u)]) {
                    dp[idx(nmask, u)] = cand;
                    parent[idx(nmask, u)] = static_cast<std::int8_t>(v);
                }
            }
        }
    }

    // Melhor estado final e reconstrução da rota pelos ponteiros "parent".
    const std::size_t all = full - 1;
    int last = 0;
    for (int v = 1; v < m; ++v)
        if (dp[idx(all, v)] < dp[idx(all, last)]) last = v;

    Result res;
    res.cost = dp[idx(all, last)];
    res.route.assign(inst.n, 0);
    std::size_t mask = all;
    for (int pos = m; pos >= 1; --pos) {
        res.route[pos] = last + 1;
        const int prev = parent[idx(mask, last)];
        mask &= ~(std::size_t{1} << last);
        last = prev;
    }
    return res;
}

// -----------------------------------------------------------------------------
// Limitante inferior via MST
// -----------------------------------------------------------------------------
// Justificativa:
//  (1) As m arestas de qualquer rota formam uma árvore geradora (um caminho
//      hamiltoniano). Propriedade de matroides: a i-ésima menor aresta de
//      QUALQUER árvore geradora é ≥ a i-ésima menor aresta da MST.
//  (2) L(π) = Σ (m-k+1)·p_k. Pela desigualdade de rearranjo, o menor valor
//      possível dessa soma, dadas as arestas do caminho, é obtido associando
//      os maiores pesos (m, m-1, ...) às menores arestas.
//  Logo, L(π) ≥ Σ (m-k+1)·p_(k) ≥ Σ (m-k+1)·mst_(k) = LB  para toda rota π.
// Matrizes assimétricas: a MST é calculada com w(u,v) = min(d(u,v), d(v,u)).
// Como toda aresta usada pela rota custa ≥ w, o argumento continua válido.
// MST por Prim em O(n²) (grafo completo, matriz densa).
Cost mst_lower_bound(const Instance& inst) {
    const int n = inst.n;
    if (n <= 1) return 0;
    constexpr Cost INF = std::numeric_limits<Cost>::max();
    std::vector<Cost> key(n, INF);
    std::vector<bool> in_tree(n, false);
    std::vector<Cost> edges;
    edges.reserve(n - 1);

    key[0] = 0;
    for (int it = 0; it < n; ++it) {
        int u = -1;
        for (int v = 0; v < n; ++v)
            if (!in_tree[v] && (u == -1 || key[v] < key[u])) u = v;
        in_tree[u] = true;
        if (it > 0) edges.push_back(key[u]);
        for (int v = 0; v < n; ++v)
            if (!in_tree[v]) key[v] = std::min(key[v], std::min(inst.d(u, v), inst.d(v, u)));
    }

    std::sort(edges.begin(), edges.end());
    const Cost m = n - 1;
    Cost lb = 0;
    for (Cost k = 1; k <= m; ++k) lb += (m - k + 1) * edges[k - 1];
    return lb;
}

}  // namespace mlp
