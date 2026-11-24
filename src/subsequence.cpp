// =============================================================================
// subsequence.cpp — Construção da tabela de subsequências
// =============================================================================
#include "mlp/subsequence.hpp"

namespace mlp {

void SubseqTable::rebuild(const Instance& inst, const Route& route) {
    n_ = static_cast<int>(route.size());
    // resize (e não assign): toda célula é sobrescrita abaixo, então não há por
    // que zerar a tabela a cada reconstrução.
    table_.resize(static_cast<std::size_t>(n_) * n_);
    auto at = [this](int i, int j) -> Subseq& { return table_[static_cast<std::size_t>(i) * n_ + j]; };

    for (int i = 0; i < n_; ++i) {
        at(i, i) = Subseq::single(route[i]);
        // Sentido direto: [i..j] = [i..j-1] ⊕ route[j]
        for (int j = i + 1; j < n_; ++j) at(i, j) = concat(inst, at(i, j - 1), Subseq::single(route[j]));
        // Sentido reverso: route[i], route[i-1], ..., route[j] = (i..j+1) ⊕ route[j]
        for (int j = i - 1; j >= 0; --j) at(i, j) = concat(inst, at(i, j + 1), Subseq::single(route[j]));
    }
}

}  // namespace mlp
