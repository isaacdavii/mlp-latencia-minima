// =============================================================================
// mlp_bench — Experimentos que geram os CSVs usados nos gráficos
// -----------------------------------------------------------------------------
// Uso: mlp_bench EXPERIMENTO --out ARQUIVO.csv [opções]
//
//   exact     Tempo e memória de pico: força bruta vs DP em função de m
//             --mmin 3 --mmax 20 --brute-max 11 --instances 3 --reps 3
//   quality   Heurísticas vs ótimo (DP) em instâncias pequenas
//             --sizes 8,10,12,14,16,18 --instances 10
//   scale     Heurísticas em instâncias maiores (GILS com várias sementes)
//             --sizes 20,50,100,200 --instances 5 --seeds 3
//   tradeoff  Custo × tempo da GILS-RVND variando I_ILS
//             --n 100 --instances 5 --seeds 3 --iils 1,2,5,10,25,50,100
//   perturb   Com que frequência double-bridge + RVND volta ao MESMO ótimo local
//             --sizes 10,20,30,60,100 --instances 5 --trials 100
//
// Padrões de --mmax e --brute-max vêm do registro de algoritmos (algorithms.cpp).
// Instâncias: make_random_euclidean(n, seed) com seed = 1..instances —
// idênticas em qualquer máquina.
// =============================================================================
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

#include "mlp/mlp.hpp"

namespace {

using namespace mlp;

struct Args {
    std::string experiment, out;
    int mmin = 3;
    int mmax = find_algorithm("dp")->practical_max;
    int brute_max = find_algorithm("brute")->practical_max;
    int instances = 3, reps = 3, n = 100, seeds = 3, trials = 100;
    std::vector<int> sizes, iils;
};

const std::vector<std::string> kHeuristics = {"nn", "nn-rvnd", "gils"};

std::vector<int> parse_list(const std::string& s) {
    std::vector<int> v;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, ',')) v.push_back(std::stoi(tok));
    return v;
}

/// Pico de memória residente do processo em MB (-1 se o SO não suportar).
/// Linux informa ru_maxrss em KB; macOS, em bytes.
double peak_rss_mb() {
#if defined(__APPLE__)
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return static_cast<double>(u.ru_maxrss) / (1024.0 * 1024.0);
#elif defined(__unix__)
    rusage u{};
    getrusage(RUSAGE_SELF, &u);
    return static_cast<double>(u.ru_maxrss) / 1024.0;
#else
    return -1.0;
#endif
}

/// Executa `name` `reps` vezes; devolve o resultado e a MEDIANA dos tempos (ms).
/// Aborta se a rota for inválida ou o custo inconsistente.
std::pair<Result, double> run(const std::string& name, const Instance& inst, const GilsParams& p, int reps) {
    std::vector<double> times;
    Result r;
    for (int i = 0; i < reps; ++i) {
        Timer t;
        r = run_algorithm(name, inst, p);
        times.push_back(t.ms());
    }
    if (!is_valid_route(inst, r.route) || latency(inst, r.route) != r.cost) {
        std::cerr << "ERRO de corretude em " << name << " (" << inst.name << ")\n";
        std::exit(2);
    }
    std::sort(times.begin(), times.end());
    return {r, times[times.size() / 2]};
}

// -----------------------------------------------------------------------------
// m crescente: como o pico de memória do processo só aumenta, o valor lido
// logo após a DP de tamanho m é o pico dessa DP (as anteriores usaram menos).
void exp_exact(const Args& a, std::ostream& out) {
    out << "m,instance,algo,time_ms,cost,peak_rss_mb\n";
    for (int m = a.mmin; m <= a.mmax; ++m) {
        for (int s = 1; s <= a.instances; ++s) {
            const Instance inst = make_random_euclidean(m + 1, s);
            auto [dp, tdp] = run("dp", inst, {}, m >= 18 ? 1 : a.reps);
            out << m << "," << s << ",dp," << tdp << "," << dp.cost << "," << peak_rss_mb() << "\n";
            if (m <= a.brute_max) {
                auto [bf, tbf] = run("brute", inst, {}, m >= 10 ? 1 : a.reps);
                if (bf.cost != dp.cost) { std::cerr << "ERRO: DP != força bruta\n"; std::exit(2); }
                out << m << "," << s << ",brute," << tbf << "," << bf.cost << ",\n";
            }
            std::cerr << "[exact] m=" << m << " inst=" << s << "\n";
        }
    }
}

// -----------------------------------------------------------------------------
void exp_quality(const Args& a, std::ostream& out) {
    out << "m,instance,algo,time_ms,cost,opt,gap_pct\n";
    for (int m : a.sizes) {
        for (int s = 1; s <= a.instances; ++s) {
            const Instance inst = make_random_euclidean(m + 1, s);
            auto [opt, topt] = run("dp", inst, {}, 1);
            auto row = [&](const std::string& name, const Result& r, double t) {
                const double gap = 100.0 * static_cast<double>(r.cost - opt.cost) / static_cast<double>(opt.cost);
                out << m << "," << s << "," << name << "," << t << "," << r.cost << "," << opt.cost << "," << gap << "\n";
            };
            row("dp", opt, topt);
            for (const auto& h : kHeuristics) {
                auto [r, t] = run(h, inst, {}, h == "gils" ? 1 : a.reps);
                row(h, r, t);
            }
            std::cerr << "[quality] m=" << m << " inst=" << s << "\n";
        }
    }
}

// -----------------------------------------------------------------------------
void exp_scale(const Args& a, std::ostream& out) {
    out << "n,instance,algo,run,time_ms,cost,lb\n";
    for (int n : a.sizes) {
        for (int s = 1; s <= a.instances; ++s) {
            const Instance inst = make_random_euclidean(n, s);
            const Cost lb = mst_lower_bound(inst);
            for (const auto& h : kHeuristics) {
                const int runs = h == "gils" ? a.seeds : 1;  // só a GILS depende da semente de forma relevante
                for (int seed = 1; seed <= runs; ++seed) {
                    GilsParams p;
                    p.seed = static_cast<std::uint64_t>(seed);
                    auto [r, t] = run(h, inst, p, h == "nn" ? a.reps : 1);
                    out << n << "," << s << "," << h << "," << (h == "gils" ? seed : 0) << "," << t << "," << r.cost
                        << "," << lb << "\n";
                }
            }
            std::cerr << "[scale] n=" << n << " inst=" << s << "\n";
        }
    }
}

// -----------------------------------------------------------------------------
void exp_tradeoff(const Args& a, std::ostream& out) {
    out << "n,instance,seed,iils,time_ms,cost\n";
    for (int s = 1; s <= a.instances; ++s) {
        const Instance inst = make_random_euclidean(a.n, s);
        for (int seed = 1; seed <= a.seeds; ++seed)
            for (int iils : a.iils) {
                GilsParams p;
                p.max_iter_ils = iils;
                p.seed = static_cast<std::uint64_t>(seed);
                auto [r, t] = run("gils", inst, p, 1);
                out << a.n << "," << s << "," << seed << "," << iils << "," << t << "," << r.cost << "\n";
            }
        std::cerr << "[tradeoff] inst=" << s << "\n";
    }
}

// -----------------------------------------------------------------------------
// Parte do ótimo local do NN+RVND e aplica `trials` vezes perturbação + RVND
// (aceitando melhoras, como no ILS); conta quantas vezes a rota final é
// idêntica à de partida.
void exp_perturb(const Args& a, std::ostream& out) {
    out << "n,instance,trials,same,improved\n";
    for (int n : a.sizes) {
        for (int s = 1; s <= a.instances; ++s) {
            const Instance inst = make_random_euclidean(n, s);
            Rng rng(static_cast<std::uint64_t>(s));
            Route r = nearest_neighbor(inst);
            Cost c = rvnd(inst, r, rng);
            int same = 0, improved = 0;
            for (int t = 0; t < a.trials; ++t) {
                Route p = double_bridge(r, rng);
                const Cost cp = rvnd(inst, p, rng);
                same += p == r;
                if (cp < c) { r = p; c = cp; ++improved; }
            }
            out << n << "," << s << "," << a.trials << "," << same << "," << improved << "\n";
        }
        std::cerr << "[perturb] n=" << n << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso: mlp_bench {exact|quality|scale|tradeoff|perturb} --out ARQ.csv [opções]\n";
        return 1;
    }
    Args a;
    a.experiment = argv[1];
    a.sizes = a.experiment == "quality"   ? std::vector<int>{8, 10, 12, 14, 16, 18}
              : a.experiment == "perturb" ? std::vector<int>{10, 20, 30, 60, 100}
                                          : std::vector<int>{20, 50, 100, 200};
    a.iils = {1, 2, 5, 10, 25, 50, 100};
    if (a.experiment == "quality") a.instances = 10;
    if (a.experiment == "scale" || a.experiment == "tradeoff" || a.experiment == "perturb") a.instances = 5;

    for (int i = 2; i < argc; ++i) {
        const std::string k = argv[i];
        if (i + 1 >= argc) { std::cerr << "faltou valor para " << k << "\n"; return 1; }
        const std::string v = argv[++i];
        if (k == "--out") a.out = v;
        else if (k == "--mmin") a.mmin = std::stoi(v);
        else if (k == "--mmax") a.mmax = std::stoi(v);
        else if (k == "--brute-max") a.brute_max = std::stoi(v);
        else if (k == "--instances") a.instances = std::stoi(v);
        else if (k == "--reps") a.reps = std::stoi(v);
        else if (k == "--sizes") a.sizes = parse_list(v);
        else if (k == "--n") a.n = std::stoi(v);
        else if (k == "--iils") a.iils = parse_list(v);
        else if (k == "--seeds") a.seeds = std::stoi(v);
        else if (k == "--trials") a.trials = std::stoi(v);
        else { std::cerr << "opção desconhecida: " << k << "\n"; return 1; }
    }

    std::ofstream file;
    if (!a.out.empty()) {
        file.open(a.out);
        if (!file) { std::cerr << "não foi possível abrir " << a.out << "\n"; return 1; }
    }
    std::ostream& out = a.out.empty() ? std::cout : file;

    if (a.experiment == "exact") exp_exact(a, out);
    else if (a.experiment == "quality") exp_quality(a, out);
    else if (a.experiment == "scale") exp_scale(a, out);
    else if (a.experiment == "tradeoff") exp_tradeoff(a, out);
    else if (a.experiment == "perturb") exp_perturb(a, out);
    else { std::cerr << "experimento desconhecido: " << a.experiment << "\n"; return 1; }
    return 0;
}
