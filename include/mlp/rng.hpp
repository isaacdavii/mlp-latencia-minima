// =============================================================================
// rng.hpp — Gerador pseudoaleatório portável e reprodutível
// -----------------------------------------------------------------------------
// Por que não usar std::uniform_int_distribution?
//   O padrão C++ define o *motor* (mt19937 etc.) de forma exata, mas NÃO define
//   o algoritmo das distribuições. libstdc++ (GCC), libc++ (Clang) e MSVC podem
//   produzir sequências diferentes para a mesma semente. Para que os
//   experimentos sejam reprodutíveis em qualquer máquina, implementamos aqui o
//   xoshiro256** (Blackman & Vigna) semeado por SplitMix64, e as distribuições
//   uniformes manualmente.
// =============================================================================
#pragma once

#include <cstdint>

namespace mlp {

class Rng {
public:
    explicit Rng(std::uint64_t seed = 42) { reseed(seed); }

    /// Reinicia o estado a partir de uma semente (via SplitMix64).
    void reseed(std::uint64_t seed) {
        for (auto& s : state_) s = splitmix64(seed);
    }

    /// Próximo inteiro de 64 bits (xoshiro256**).
    std::uint64_t next() {
        const std::uint64_t result = rotl(state_[1] * 5, 7) * 9;
        const std::uint64_t t = state_[1] << 17;
        state_[2] ^= state_[0];
        state_[3] ^= state_[1];
        state_[1] ^= state_[2];
        state_[0] ^= state_[3];
        state_[2] ^= t;
        state_[3] = rotl(state_[3], 45);
        return result;
    }

    /// Inteiro uniforme em [lo, hi] (inclusivo), sem viés (rejeição).
    long long uniform_int(long long lo, long long hi) {
        const std::uint64_t range = static_cast<std::uint64_t>(hi - lo) + 1;
        if (range == 0) return lo + static_cast<long long>(next());  // intervalo completo
        const std::uint64_t limit = UINT64_MAX - (UINT64_MAX % range);
        std::uint64_t x;
        do { x = next(); } while (x >= limit);
        return lo + static_cast<long long>(x % range);
    }

    /// Real uniforme em [0, 1).
    double uniform01() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }

private:
    std::uint64_t state_[4]{};

    static std::uint64_t rotl(std::uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
    static std::uint64_t splitmix64(std::uint64_t& x) {
        std::uint64_t z = (x += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
};

}  // namespace mlp
