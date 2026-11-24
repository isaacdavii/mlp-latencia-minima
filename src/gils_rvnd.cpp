// =============================================================================
// gils_rvnd.cpp — Implementação da GILS-RVND
// =============================================================================
#include "mlp/gils_rvnd.hpp"

#include <algorithm>
#include <limits>

#include "mlp/construction.hpp"
#include "mlp/local_search.hpp"
#include "mlp/rng.hpp"

namespace mlp {

Result gils_rvnd(const Instance& inst, const GilsParams& params) {
    Rng rng(params.seed);
    const int iter_ils = params.max_iter_ils > 0 ? params.max_iter_ils : std::min(inst.n, 100);

    Result best;
    best.cost = std::numeric_limits<Cost>::max();

    for (int iter = 0; iter < params.max_iter; ++iter) {
        // α ∈ {0.00, 0.01, ..., 0.25}: RCL pequena ⇒ construção quase gulosa,
        // mas com diversidade suficiente entre os reinícios.
        const double alpha = static_cast<double>(rng.uniform_int(0, 25)) / 100.0;
        Route s = grasp_construction(inst, alpha, rng);
        Route s_best = s;
        Cost f_best = latency(inst, s);

        int no_improve = 0;
        while (no_improve < iter_ils) {
            const Cost f = rvnd(inst, s, rng);
            if (f < f_best) {
                s_best = s;
                f_best = f;
                no_improve = 0;
            }
            s = double_bridge(s_best, rng);
            ++no_improve;
        }

        if (f_best < best.cost) best = {s_best, f_best};
    }
    return best;
}

}  // namespace mlp
