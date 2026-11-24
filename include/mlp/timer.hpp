// =============================================================================
// timer.hpp — Medição de tempo de parede com relógio monotônico
// =============================================================================
#pragma once

#include <chrono>

namespace mlp {

class Timer {
public:
    Timer() : start_(Clock::now()) {}
    void reset() { start_ = Clock::now(); }
    /// Milissegundos desde a construção / último reset.
    double ms() const { return std::chrono::duration<double, std::milli>(Clock::now() - start_).count(); }

private:
    using Clock = std::chrono::steady_clock;  // monotônico: imune a ajustes do relógio do SO
    Clock::time_point start_;
};

}  // namespace mlp
