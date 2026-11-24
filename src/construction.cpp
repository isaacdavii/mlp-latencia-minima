// =============================================================================
// construction.cpp — Vizinho mais próximo e construção GRASP
// =============================================================================
#include "mlp/construction.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace mlp {

Route nearest_neighbor(const Instance& inst) {
    Route route{0};
    route.reserve(inst.n);
    std::vector<bool> visited(inst.n, false);
    visited[0] = true;

    int cur = 0;
    for (int step = 1; step < inst.n; ++step) {
        int best = -1;
        for (int v = 1; v < inst.n; ++v)
            if (!visited[v] && (best == -1 || inst.d(cur, v) < inst.d(cur, best))) best = v;
        visited[best] = true;
        route.push_back(best);
        cur = best;
    }
    return route;
}

Result solve_nn(const Instance& inst) {
    Route r = nearest_neighbor(inst);
    const Cost c = latency(inst, r);
    return {std::move(r), c};
}

Route grasp_construction(const Instance& inst, double alpha, Rng& rng) {
    Route route{0};
    route.reserve(inst.n);
    std::vector<int> candidates;  // CL: clientes ainda não inseridos
    for (int v = 1; v < inst.n; ++v) candidates.push_back(v);

    int cur = 0;
    while (!candidates.empty()) {
        // Ordena CL pela distância ao último vértice inserido.
        std::sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            const Cost da = inst.d(cur, a), db = inst.d(cur, b);
            return da != db ? da < db : a < b;  // desempate determinístico
        });
        // Tamanho da RCL: pelo menos 1.
        const int rcl = std::max(1, static_cast<int>(std::ceil(alpha * static_cast<double>(candidates.size()))));
        const int pick = static_cast<int>(rng.uniform_int(0, rcl - 1));
        cur = candidates[pick];
        route.push_back(cur);
        candidates.erase(candidates.begin() + pick);
    }
    return route;
}

}  // namespace mlp
