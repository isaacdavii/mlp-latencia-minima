// =============================================================================
// local_search.cpp — Vizinhanças com avaliação O(1), RVND e double-bridge
// =============================================================================
#include "mlp/local_search.hpp"

#include <algorithm>
#include <utility>
#include <vector>

#include "mlp/construction.hpp"

namespace mlp {

std::string to_string(Neighborhood nb) {
    switch (nb) {
        case Neighborhood::Swap: return "swap";
        case Neighborhood::TwoOpt: return "2-opt";
        case Neighborhood::OrOpt1: return "or-opt-1";
        case Neighborhood::OrOpt2: return "or-opt-2";
        case Neighborhood::OrOpt3: return "or-opt-3";
    }
    return "?";
}

// -----------------------------------------------------------------------------
// Avaliação O(1) por concatenação
// -----------------------------------------------------------------------------
// L = última posição da rota. fwd(a, b) com a > b é vazia, o que trata
// automaticamente os casos de borda (ex.: j == L ⇒ cauda vazia).

Cost eval_swap(const Instance& inst, const SubseqTable& t, int i, int j) {
    const int L = t.size() - 1;
    Subseq s = t.fwd(0, i - 1);
    s = concat(inst, s, t.fwd(j, j));
    s = concat(inst, s, t.fwd(i + 1, j - 1));  // vazia se j == i + 1
    s = concat(inst, s, t.fwd(i, i));
    s = concat(inst, s, t.fwd(j + 1, L));
    return s.C;
}

Cost eval_two_opt(const Instance& inst, const SubseqTable& t, int i, int j) {
    const int L = t.size() - 1;
    Subseq s = concat(inst, t.fwd(0, i - 1), t.rev(i, j));
    s = concat(inst, s, t.fwd(j + 1, L));
    return s.C;
}

Cost eval_or_opt(const Instance& inst, const SubseqTable& t, int i, int k, int p) {
    const int L = t.size() - 1;
    const int e = i + k - 1;  // fim do bloco
    Subseq s;
    if (p < i) {  // bloco vai para trás:  [0..p-1] [bloco] [p..i-1] [e+1..L]
        s = concat(inst, t.fwd(0, p - 1), t.fwd(i, e));
        s = concat(inst, s, t.fwd(p, i - 1));
        s = concat(inst, s, t.fwd(e + 1, L));
    } else {      // bloco vai para frente: [0..i-1] [e+1..p-1] [bloco] [p..L]
        s = concat(inst, t.fwd(0, i - 1), t.fwd(e + 1, p - 1));
        s = concat(inst, s, t.fwd(i, e));
        s = concat(inst, s, t.fwd(p, L));
    }
    return s.C;
}

// -----------------------------------------------------------------------------
// Aplicação dos movimentos
// -----------------------------------------------------------------------------
void apply_swap(Route& r, int i, int j) { std::swap(r[i], r[j]); }

void apply_two_opt(Route& r, int i, int j) { std::reverse(r.begin() + i, r.begin() + j + 1); }

void apply_or_opt(Route& r, int i, int k, int p) {
    if (p < i) std::rotate(r.begin() + p, r.begin() + i, r.begin() + i + k);
    else       std::rotate(r.begin() + i, r.begin() + i + k, r.begin() + p);
}

// -----------------------------------------------------------------------------
// Best improvement em uma vizinhança
// -----------------------------------------------------------------------------
bool best_improvement(const Instance& inst, Route& route, Cost& cost, Neighborhood nb, const SubseqTable& t) {
    const int n = static_cast<int>(route.size());
    Cost best = cost;
    int bi = -1, bj = -1;  // melhor movimento encontrado (bj = p no or-opt)

    switch (nb) {
        case Neighborhood::Swap:
            for (int i = 1; i < n - 1; ++i)
                for (int j = i + 1; j < n; ++j) {
                    const Cost c = eval_swap(inst, t, i, j);
                    if (c < best) { best = c; bi = i; bj = j; }
                }
            if (bi < 0) return false;
            apply_swap(route, bi, bj);
            break;

        case Neighborhood::TwoOpt:
            for (int i = 1; i < n - 1; ++i)
                for (int j = i + 1; j < n; ++j) {
                    const Cost c = eval_two_opt(inst, t, i, j);
                    if (c < best) { best = c; bi = i; bj = j; }
                }
            if (bi < 0) return false;
            apply_two_opt(route, bi, bj);
            break;

        case Neighborhood::OrOpt1:
        case Neighborhood::OrOpt2:
        case Neighborhood::OrOpt3: {
            const int k = nb == Neighborhood::OrOpt1 ? 1 : nb == Neighborhood::OrOpt2 ? 2 : 3;
            for (int i = 1; i + k - 1 < n; ++i)
                for (int p = 1; p <= n; ++p) {
                    if (p >= i && p <= i + k) continue;  // posições que não mudam a rota
                    const Cost c = eval_or_opt(inst, t, i, k, p);
                    if (c < best) { best = c; bi = i; bj = p; }
                }
            if (bi < 0) return false;
            apply_or_opt(route, bi, k, bj);
            break;
        }
    }
    cost = best;
    return true;
}

// -----------------------------------------------------------------------------
// RVND
// -----------------------------------------------------------------------------
Cost rvnd(const Instance& inst, Route& route, Rng& rng) {
    Cost cost = latency(inst, route);
    if (inst.n <= 2) return cost;  // 0 ou 1 cliente: nada a melhorar

    SubseqTable table(inst, route);
    std::vector<Neighborhood> nl(kAllNeighborhoods.begin(), kAllNeighborhoods.end());

    while (!nl.empty()) {
        const int pick = static_cast<int>(rng.uniform_int(0, static_cast<long long>(nl.size()) - 1));
        if (best_improvement(inst, route, cost, nl[pick], table)) {
            table.rebuild(inst, route);  // O(n²) — só após uma melhora
            nl.assign(kAllNeighborhoods.begin(), kAllNeighborhoods.end());
        } else {
            nl.erase(nl.begin() + pick);
        }
    }
    return cost;
}

Result nn_rvnd(const Instance& inst, std::uint64_t seed) {
    Rng rng(seed);
    Route r = nearest_neighbor(inst);
    const Cost c = rvnd(inst, r, rng);
    return {std::move(r), c};
}

// -----------------------------------------------------------------------------
// Perturbação double-bridge
// -----------------------------------------------------------------------------
Route double_bridge(const Route& route, Rng& rng) {
    const int m = static_cast<int>(route.size()) - 1;  // clientes
    Route r = route;
    if (m < 2) return r;
    if (m < 4) {  // rota pequena demais para dois segmentos: troca aleatória simples
        const int i = static_cast<int>(rng.uniform_int(1, m - 1));
        apply_swap(r, i, i + 1);
        return r;
    }

    const int maxlen = std::max(1, m / 10);
    // Sorteia dois segmentos disjuntos [a, a+l1-1] e [b, b+l2-1], com a+l1 ≤ b.
    // Como l1 + l2 ≤ 2·max(1, m/10) ≤ m, os intervalos de sorteio nunca são vazios.
    const int l1 = static_cast<int>(rng.uniform_int(1, maxlen));
    const int l2 = static_cast<int>(rng.uniform_int(1, maxlen));
    const int a = static_cast<int>(rng.uniform_int(1, m - l1 - l2 + 1));
    const int b = static_cast<int>(rng.uniform_int(a + l1, m - l2 + 1));

    // Nova ordem: [0..a-1] [seg2] [a+l1..b-1] [seg1] [b+l2..m]
    Route out;
    out.reserve(r.size());
    out.insert(out.end(), r.begin(), r.begin() + a);
    out.insert(out.end(), r.begin() + b, r.begin() + b + l2);
    out.insert(out.end(), r.begin() + a + l1, r.begin() + b);
    out.insert(out.end(), r.begin() + a, r.begin() + a + l1);
    out.insert(out.end(), r.begin() + b + l2, r.end());
    return out;
}

}  // namespace mlp
