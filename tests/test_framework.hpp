// =============================================================================
// test_framework.hpp — Mini framework de testes (sem dependências externas)
// -----------------------------------------------------------------------------
//   TEST(nome) { CHECK(cond); CHECK_EQ(a, b); }
// Cada TEST se registra automaticamente; run_all() executa todos, imprime um
// resumo e retorna 0 se tudo passou (código usado pelo CTest).
// =============================================================================
#pragma once

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace testing {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}
inline int& failures() {
    static int f = 0;
    return f;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

inline int run_all(const std::string& filter = "") {
    int failed_tests = 0, ran = 0;
    for (const auto& t : registry()) {
        if (!filter.empty() && t.name.find(filter) == std::string::npos) continue;
        ++ran;
        const int before = failures();
        try {
            t.fn();
        } catch (const std::exception& e) {
            ++failures();
            std::cout << "    exceção: " << e.what() << "\n";
        }
        const bool ok = failures() == before;
        if (!ok) ++failed_tests;
        std::cout << (ok ? "[ OK ]   " : "[FALHA] ") << t.name << "\n";
    }
    std::cout << "\n" << (ran - failed_tests) << "/" << ran << " testes passaram\n";
    return failed_tests == 0 ? 0 : 1;
}

}  // namespace testing

#define TEST(name)                                                      \
    static void name();                                                 \
    static ::testing::Registrar registrar_##name(#name, name);         \
    static void name()

#define CHECK(cond)                                                                         \
    do {                                                                                    \
        if (!(cond)) {                                                                      \
            ++::testing::failures();                                                        \
            std::cout << "    " << __FILE__ << ":" << __LINE__ << ": CHECK(" #cond ") falhou\n"; \
        }                                                                                   \
    } while (0)

#define CHECK_EQ(a, b)                                                                            \
    do {                                                                                          \
        const auto va_ = (a);                                                                     \
        const auto vb_ = (b);                                                                     \
        if (!(va_ == vb_)) {                                                                      \
            ++::testing::failures();                                                              \
            std::cout << "    " << __FILE__ << ":" << __LINE__ << ": CHECK_EQ(" #a ", " #b ") — " \
                      << va_ << " != " << vb_ << "\n";                                            \
        }                                                                                         \
    } while (0)
