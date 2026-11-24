// =============================================================================
// subsequence.hpp — Concatenação de subsequências (avaliação de movimentos O(1))
// -----------------------------------------------------------------------------
// Ideia (Silva et al., 2012; Vidal et al., 2013): para cada subsequência σ da
// rota guardamos três valores:
//
//   D(σ) = duração: tempo para percorrer σ do primeiro ao último vértice
//   C(σ) = custo:   soma dos tempos de chegada em σ, com relógio iniciando em
//                   0 no primeiro vértice de σ
//   W(σ) = peso:    nº de vértices de σ (quantos "sofrem" um atraso)
//
// Concatenação σ = σ1 ⊕ σ2  (t = D1 + d(último(σ1), primeiro(σ2))):
//   D = t + D2
//   C = C1 + W2 · t + C2      ← todos os W2 vértices de σ2 atrasam t
//   W = W1 + W2
//
// Com a tabela de todas as subsequências da rota atual pré-computada em O(n²),
// o custo de QUALQUER movimento (swap, 2-opt, or-opt) é obtido concatenando no
// máximo 5 pedaços: O(1) por vizinho em vez de O(n) da reavaliação completa.
// =============================================================================
#pragma once

#include <vector>

#include "mlp/instance.hpp"
#include "mlp/solution.hpp"

namespace mlp {

struct Subseq {
    Cost D = 0;      ///< duração
    Cost C = 0;      ///< custo acumulado (soma das chegadas)
    int W = 0;       ///< nº de vértices (0 ⇒ subsequência vazia)
    int first = -1;  ///< primeiro vértice
    int last = -1;   ///< último vértice

    /// Subsequência de um único vértice.
    static Subseq single(int v) { return {0, 0, 1, v, v}; }
    bool empty() const { return W == 0; }
};

/// σ1 ⊕ σ2 — O(1). Vazias são elemento neutro.
inline Subseq concat(const Instance& inst, const Subseq& a, const Subseq& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    const Cost t = a.D + inst.d(a.last, b.first);
    return {t + b.D, a.C + static_cast<Cost>(b.W) * t + b.C, a.W + b.W, a.first, b.last};
}

/// Tabela com todas as subsequências da rota (sentido direto e reverso).
///   fwd(i, j), i ≤ j : route[i], route[i+1], ..., route[j]
///   rev(i, j), i ≤ j : route[j], route[j-1], ..., route[i]   (para o 2-opt)
/// Construção O(n²); consulta O(1).
class SubseqTable {
public:
    SubseqTable(const Instance& inst, const Route& route) : n_(static_cast<int>(route.size())) {
        rebuild(inst, route);
    }

    /// Recalcula a tabela inteira após a rota mudar — O(n²).
    void rebuild(const Instance& inst, const Route& route);

    /// Subsequência direta [i..j]; vazia se i > j ou fora dos limites.
    const Subseq& fwd(int i, int j) const {
        if (i > j || i >= n_ || j < 0) return kEmpty;
        return table_[static_cast<std::size_t>(i) * n_ + j];
    }
    /// Subsequência [i..j] percorrida ao contrário.
    const Subseq& rev(int i, int j) const {
        if (i > j) return kEmpty;
        return table_[static_cast<std::size_t>(j) * n_ + i];
    }
    int size() const { return n_; }

private:
    int n_;
    std::vector<Subseq> table_;  // (i, j) com i ≤ j: direto; i > j: reverso de j..i
    static inline const Subseq kEmpty{};
};

}  // namespace mlp
