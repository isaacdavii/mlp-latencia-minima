// =============================================================================
// mlp_gen — Gera instâncias euclidianas aleatórias no formato TSPLIB
// -----------------------------------------------------------------------------
// Uso: mlp_gen N SEED ARQUIVO_SAIDA
// A mesma (N, SEED) gera sempre a mesma instância (ver rng.hpp).
// =============================================================================
#include <iostream>
#include <string>

#include "mlp/instance.hpp"

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Uso: mlp_gen N SEED ARQUIVO_SAIDA\n";
        return 1;
    }
    try {
        const int n = std::stoi(argv[1]);
        const auto seed = std::stoull(argv[2]);
        const mlp::Instance inst = mlp::make_random_euclidean(n, seed);
        mlp::save_tsplib(inst, argv[3]);
        std::cout << "gerado " << argv[3] << " (" << inst.name << ")\n";
    } catch (const std::exception& e) {
        std::cerr << "erro: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
