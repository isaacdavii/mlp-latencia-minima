// =============================================================================
// mlp_solver — Resolve uma instância com o algoritmo escolhido
// -----------------------------------------------------------------------------
// Uso:
//   mlp_solver --instance arquivo.tsp [--algo ALG] [--seed S] [--imax N] [--iils N] [--route]
//   mlp_solver --random N [--gen-seed S] [--algo ALG] ...
//
// ALG ∈ { brute, dp, nn, nn-rvnd, gils, all }   (padrão: all)
// "all" executa, em ordem, todos cujo limite prático comporta a instância
// (limites definidos uma única vez em src/algorithms.cpp).
// =============================================================================
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "mlp/mlp.hpp"

namespace {

void usage() {
    std::cerr << "Uso:\n"
                 "  mlp_solver --instance ARQ.tsp [opções]\n"
                 "  mlp_solver --random N [--gen-seed S] [opções]\n\n"
                 "Opções:\n"
                 "  --algo ALG     brute | dp | nn | nn-rvnd | gils | all   (padrão: all)\n"
                 "  --seed S       semente da GILS-RVND / RVND              (padrão: 1)\n"
                 "  --imax N       I_max da GILS-RVND                       (padrão: 10)\n"
                 "  --iils N       I_ILS da GILS-RVND (padrão: min(n,100))\n"
                 "  --route        imprime a rota encontrada\n";
}

void report(const mlp::Instance& inst, const std::string& algo, const mlp::Result& r, double ms, bool show_route) {
    if (!mlp::is_valid_route(inst, r.route) || mlp::latency(inst, r.route) != r.cost) {
        std::cerr << "ERRO: " << algo << " retornou rota inválida ou custo inconsistente\n";
        std::exit(2);
    }
    std::cout << std::left << std::setw(10) << algo << " custo = " << std::setw(12) << r.cost << " tempo = " << std::fixed
              << std::setprecision(3) << ms << " ms\n";
    if (show_route) std::cout << "           rota: " << mlp::route_to_string(r.route) << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::string path, algo = "all";
    int random_n = 0;
    std::uint64_t gen_seed = 1;
    mlp::GilsParams gp;
    bool show_route = false;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) { usage(); std::exit(1); }
            return argv[++i];
        };
        if (a == "--instance") path = next();
        else if (a == "--random") random_n = std::stoi(next());
        else if (a == "--gen-seed") gen_seed = std::stoull(next());
        else if (a == "--algo") algo = next();
        else if (a == "--seed") gp.seed = std::stoull(next());
        else if (a == "--imax") gp.max_iter = std::stoi(next());
        else if (a == "--iils") gp.max_iter_ils = std::stoi(next());
        else if (a == "--route") show_route = true;
        else if (a == "-h" || a == "--help") { usage(); return 0; }
        else { std::cerr << "opção desconhecida: " << a << "\n"; usage(); return 1; }
    }
    if (path.empty() && random_n <= 0) { usage(); return 1; }
    if (algo != "all" && !mlp::find_algorithm(algo)) {
        std::cerr << "algoritmo desconhecido: " << algo << "\n";
        usage();
        return 1;
    }

    try {
        const mlp::Instance inst = path.empty() ? mlp::make_random_euclidean(random_n, gen_seed) : mlp::load_tsplib(path);
        const int m = inst.customers();
        std::cout << "Instância: " << inst.name << "  (n = " << inst.n << ", clientes = " << m << ")\n";
        std::cout << "Limitante inferior (MST): " << mlp::mst_lower_bound(inst) << "\n\n";

        for (const mlp::Algorithm& a : mlp::all_algorithms()) {
            const bool chosen = algo == a.name || (algo == "all" && (a.practical_max < 0 || m <= a.practical_max));
            if (!chosen) continue;
            mlp::Timer t;
            const mlp::Result r = a.run(inst, gp);
            report(inst, a.name, r, t.ms(), show_route);
        }
    } catch (const std::exception& e) {
        std::cerr << "erro: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
