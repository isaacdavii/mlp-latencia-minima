// =============================================================================
// solution.cpp — Avaliação e validação de rotas
// =============================================================================
#include "mlp/solution.hpp"

#include <numeric>
#include <sstream>

namespace mlp {

Cost latency(const Instance& inst, const Route& route) {
    Cost time = 0, total = 0;
    for (std::size_t k = 1; k < route.size(); ++k) {
        time += inst.d(route[k - 1], route[k]);  // chegada no k-ésimo cliente
        total += time;
    }
    return total;
}

bool is_valid_route(const Instance& inst, const Route& route) {
    if (static_cast<int>(route.size()) != inst.n || route.empty() || route[0] != 0) return false;
    std::vector<bool> seen(inst.n, false);
    for (int v : route) {
        if (v < 0 || v >= inst.n || seen[v]) return false;
        seen[v] = true;
    }
    return true;
}

Route identity_route(int n) {
    Route r(n);
    std::iota(r.begin(), r.end(), 0);
    return r;
}

std::string route_to_string(const Route& route) {
    std::ostringstream os;
    for (std::size_t i = 0; i < route.size(); ++i) os << (i ? " -> " : "") << route[i];
    return os.str();
}

}  // namespace mlp
