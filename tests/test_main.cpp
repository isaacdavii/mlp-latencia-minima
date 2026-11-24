// =============================================================================
// test_main.cpp — Testes de corretude
// -----------------------------------------------------------------------------
// Estratégia: cada componente é validado contra uma referência independente.
//   • latência             vs. cálculo manual em exemplo pequeno
//   • concatenação O(1)    vs. latency() (avaliação direta, O(n))
//   • eval_* de cada movimento vs. latency(apply_*(rota)) em TODOS os vizinhos
//   • DP                   vs. força bruta (centenas de instâncias)
//   • limitante MST        ≤ ótimo
//   • heurísticas          rota válida, custo consistente, ≥ ótimo
//   • reprodutibilidade    mesma semente ⇒ mesmo resultado
// Uso: mlp_tests [filtro]
// =============================================================================
#include <cstdio>
#include <fstream>
#include <set>

#include "mlp/mlp.hpp"
#include "test_framework.hpp"

using namespace mlp;

namespace {

/// Rota aleatória válida (depósito na posição 0 + permutação de Fisher–Yates).
Route random_route(int n, Rng& rng) {
    Route r = identity_route(n);
    for (int i = n - 1; i > 1; --i) std::swap(r[i], r[rng.uniform_int(1, i)]);
    return r;
}

/// Instância assimétrica aleatória: d(i,j) e d(j,i) sorteados independentemente.
Instance asymmetric_instance(int n, std::uint64_t seed) {
    Rng rng(seed);
    std::vector<std::vector<Cost>> m(n, std::vector<Cost>(n, 0));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            if (i != j) m[i][j] = rng.uniform_int(1, 100);
    return from_matrix(m, "asym");
}

/// Grava `text` em `path` (para testar a leitura de arquivos malformados).
void write_file(const std::string& path, const std::string& text) {
    std::ofstream(path) << text;
}

/// true se load_tsplib(path) lança exceção.
bool load_throws(const std::string& path) {
    try {
        load_tsplib(path);
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

/// Instância "em linha": vértices em 0, 1, 2, ..., n-1 sobre a reta.
Instance line_instance(int n) {
    std::vector<std::vector<Cost>> m(n, std::vector<Cost>(n));
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) m[i][j] = i > j ? i - j : j - i;
    return from_matrix(m, "line");
}

}  // namespace

// --------------------------------------------------------------------------- instância
TEST(instancia_gerador_eh_deterministico) {
    const Instance a = make_random_euclidean(30, 7), b = make_random_euclidean(30, 7), c = make_random_euclidean(30, 8);
    CHECK(a.dist == b.dist);
    CHECK(a.dist != c.dist);
}

TEST(instancia_matriz_simetrica_e_diagonal_zero) {
    const Instance inst = make_random_euclidean(25, 3);
    for (int i = 0; i < inst.n; ++i) {
        CHECK_EQ(inst.d(i, i), 0);
        for (int j = 0; j < inst.n; ++j) CHECK_EQ(inst.d(i, j), inst.d(j, i));
    }
}

TEST(instancia_tsplib_ida_e_volta) {
    const Instance a = make_random_euclidean(15, 11);
    const std::string path = "test_roundtrip.tsp";
    save_tsplib(a, path);
    const Instance b = load_tsplib(path);
    std::remove(path.c_str());
    CHECK_EQ(a.n, b.n);
    CHECK(a.dist == b.dist);
}

// --------------------------------------------------------------------------- latência
TEST(latencia_exemplo_manual) {
    // Reta 0-1-2-3: rota 0,1,2,3 ⇒ chegadas 1, 2, 3 ⇒ L = 6
    //               rota 0,3,2,1 ⇒ chegadas 3, 4, 5 ⇒ L = 12
    const Instance inst = line_instance(4);
    CHECK_EQ(latency(inst, Route{0, 1, 2, 3}), 6);
    CHECK_EQ(latency(inst, Route{0, 3, 2, 1}), 12);
    CHECK_EQ(latency(inst, Route{0, 2, 1, 3}), 2 + 3 + 5);
}

TEST(latencia_formula_reformulada) {
    // L = Σ (m - k + 1) · d(π_{k-1}, π_k)  deve coincidir com a definição.
    Rng rng(5);
    for (int t = 0; t < 50; ++t) {
        const Instance inst = make_random_euclidean(12, 100 + t);
        const Route r = random_route(inst.n, rng);
        const int m = inst.customers();
        Cost alt = 0;
        for (int k = 1; k <= m; ++k) alt += static_cast<Cost>(m - k + 1) * inst.d(r[k - 1], r[k]);
        CHECK_EQ(latency(inst, r), alt);
    }
}

TEST(rota_validacao) {
    const Instance inst = line_instance(4);
    CHECK(is_valid_route(inst, Route{0, 2, 1, 3}));
    CHECK(!is_valid_route(inst, Route{1, 0, 2, 3}));  // não começa no depósito
    CHECK(!is_valid_route(inst, Route{0, 1, 1, 3}));  // repetido
    CHECK(!is_valid_route(inst, Route{0, 1, 2}));     // incompleta
}

// --------------------------------------------------------------------------- subsequências
TEST(subsequencias_rota_inteira_igual_latencia) {
    Rng rng(9);
    for (int t = 0; t < 50; ++t) {
        const Instance inst = make_random_euclidean(2 + t % 20, 200 + t);
        const Route r = random_route(inst.n, rng);
        const SubseqTable tab(inst, r);
        CHECK_EQ(tab.fwd(0, inst.n - 1).C, latency(inst, r));
    }
}

TEST(subsequencias_reversas) {
    Rng rng(10);
    const Instance inst = make_random_euclidean(15, 3);
    const Route r = random_route(inst.n, rng);
    const SubseqTable tab(inst, r);
    for (int i = 0; i < inst.n; ++i)
        for (int j = i; j < inst.n; ++j) {
            // Monta a sequência reversa explicitamente e compara D e C.
            Subseq s;
            for (int k = j; k >= i; --k) s = concat(inst, s, Subseq::single(r[k]));
            CHECK_EQ(tab.rev(i, j).C, s.C);
            CHECK_EQ(tab.rev(i, j).D, s.D);
        }
}

// --------------------------------------------------------------------------- vizinhanças
// Para cada movimento possível: custo previsto em O(1) == custo real após aplicar.
TEST(vizinhanca_swap_avaliacao_O1_exata) {
    Rng rng(21);
    for (int t = 0; t < 10; ++t) {
        const Instance inst = make_random_euclidean(3 + t, 300 + t);
        const Route r = random_route(inst.n, rng);
        const SubseqTable tab(inst, r);
        for (int i = 1; i < inst.n - 1; ++i)
            for (int j = i + 1; j < inst.n; ++j) {
                Route r2 = r;
                apply_swap(r2, i, j);
                CHECK_EQ(eval_swap(inst, tab, i, j), latency(inst, r2));
            }
    }
}

TEST(vizinhanca_2opt_avaliacao_O1_exata) {
    Rng rng(22);
    for (int t = 0; t < 10; ++t) {
        const Instance inst = make_random_euclidean(3 + t, 400 + t);
        const Route r = random_route(inst.n, rng);
        const SubseqTable tab(inst, r);
        for (int i = 1; i < inst.n - 1; ++i)
            for (int j = i + 1; j < inst.n; ++j) {
                Route r2 = r;
                apply_two_opt(r2, i, j);
                CHECK_EQ(eval_two_opt(inst, tab, i, j), latency(inst, r2));
            }
    }
}

TEST(vizinhanca_oropt_avaliacao_O1_exata) {
    Rng rng(23);
    for (int t = 0; t < 10; ++t) {
        const Instance inst = make_random_euclidean(4 + t, 500 + t);
        const Route r = random_route(inst.n, rng);
        const SubseqTable tab(inst, r);
        for (int k = 1; k <= 3; ++k)
            for (int i = 1; i + k - 1 < inst.n; ++i)
                for (int p = 1; p <= inst.n; ++p) {
                    if (p >= i && p <= i + k) continue;
                    Route r2 = r;
                    apply_or_opt(r2, i, k, p);
                    CHECK(is_valid_route(inst, r2));
                    CHECK_EQ(eval_or_opt(inst, tab, i, k, p), latency(inst, r2));
                }
    }
}

TEST(rvnd_produz_otimo_local) {
    // Após o RVND nenhuma vizinhança pode ter vizinho melhor.
    Rng rng(31);
    for (int t = 0; t < 5; ++t) {
        const Instance inst = make_random_euclidean(30, 600 + t);
        Route r = random_route(inst.n, rng);
        const Cost before = latency(inst, r);
        Cost c = rvnd(inst, r, rng);
        CHECK(c <= before);
        CHECK_EQ(c, latency(inst, r));
        const SubseqTable tab(inst, r);
        for (Neighborhood nb : kAllNeighborhoods) {
            Route copy = r;
            Cost cc = c;
            CHECK(!best_improvement(inst, copy, cc, nb, tab));
        }
    }
}

TEST(double_bridge_mantem_rota_valida) {
    Rng rng(41);
    for (int n : {2, 3, 4, 5, 10, 50, 120}) {
        const Instance inst = make_random_euclidean(n, n);
        Route r = random_route(n, rng);
        for (int t = 0; t < 50; ++t) {
            r = double_bridge(r, rng);
            CHECK(is_valid_route(inst, r));
        }
    }
}

// --------------------------------------------------------------------------- exatos
TEST(dp_igual_forca_bruta) {
    for (int m = 0; m <= 9; ++m)
        for (std::uint64_t s = 1; s <= 25; ++s) {
            const Instance inst = make_random_euclidean(m + 1, 1000 * m + s);
            const Result bf = solve_brute_force(inst);
            const Result dp = solve_dp(inst);
            CHECK_EQ(dp.cost, bf.cost);
            CHECK(is_valid_route(inst, dp.route));
            CHECK_EQ(latency(inst, dp.route), dp.cost);  // rota reconstruída é coerente
        }
}

TEST(dp_instancia_em_linha) {
    // Na reta, partindo da ponta, o ótimo é visitar em ordem: L = 1+2+...+m.
    const Instance inst = line_instance(10);
    CHECK_EQ(solve_dp(inst).cost, 45);
    CHECK_EQ(solve_brute_force(inst).cost, 45);
}

TEST(limitante_mst_nao_supera_otimo) {
    for (std::uint64_t s = 1; s <= 40; ++s) {
        const Instance inst = make_random_euclidean(3 + static_cast<int>(s % 10), 2000 + s);
        CHECK(mst_lower_bound(inst) <= solve_dp(inst).cost);
    }
    const Instance line = line_instance(10);
    CHECK_EQ(mst_lower_bound(line), 45);  // na reta o limitante é justo
}

// --------------------------------------------------------------------------- heurísticas
TEST(heuristicas_rotas_validas_e_custos_consistentes) {
    for (int n : {1, 2, 3, 8, 40, 90}) {
        const Instance inst = make_random_euclidean(n, 77 + n);
        const Route nn = nearest_neighbor(inst);
        CHECK(is_valid_route(inst, nn));
        Rng rng(1);
        CHECK(is_valid_route(inst, grasp_construction(inst, 0.2, rng)));
        const Result a = nn_rvnd(inst, 1);
        CHECK(is_valid_route(inst, a.route));
        CHECK_EQ(a.cost, latency(inst, a.route));
        const Result g = gils_rvnd(inst, {3, 10, 1});
        CHECK(is_valid_route(inst, g.route));
        CHECK_EQ(g.cost, latency(inst, g.route));
        // Obs.: NÃO verificamos g.cost <= custo do NN — a GILS sorteia α e não
        // necessariamente parte do NN, então isso não é uma propriedade garantida.
    }
}

TEST(grasp_alpha_zero_igual_vizinho_mais_proximo) {
    const Instance inst = make_random_euclidean(40, 5);
    Rng rng(1);
    CHECK(grasp_construction(inst, 0.0, rng) == nearest_neighbor(inst));
}

TEST(gils_nunca_abaixo_do_otimo_e_acha_otimo_pequeno) {
    int hits = 0, total = 0;
    for (int m = 5; m <= 12; ++m)
        for (std::uint64_t s = 1; s <= 5; ++s) {
            const Instance inst = make_random_euclidean(m + 1, 3000 + 10 * m + s);
            const Cost opt = solve_dp(inst).cost;
            const Cost g = gils_rvnd(inst, {10, -1, 1}).cost;
            CHECK(g >= opt);
            hits += (g == opt);
            ++total;
        }
    std::cout << "    GILS-RVND achou o ótimo em " << hits << "/" << total << " instâncias pequenas\n";
    CHECK(hits >= total * 9 / 10);
}

TEST(reprodutibilidade_mesma_semente) {
    const Instance inst = make_random_euclidean(60, 9);
    const Result a = gils_rvnd(inst, {3, 20, 123});
    const Result b = gils_rvnd(inst, {3, 20, 123});
    CHECK(a.route == b.route);
    CHECK_EQ(a.cost, b.cost);
}

TEST(rng_uniforme_dentro_do_intervalo) {
    Rng rng(1);
    std::set<long long> seen;
    for (int i = 0; i < 10000; ++i) {
        const long long v = rng.uniform_int(-3, 3);
        CHECK(v >= -3 && v <= 3);
        seen.insert(v);
    }
    CHECK_EQ(seen.size(), std::size_t{7});
}

// --------------------------------------------------------------------------- casos extremos
TEST(extremo_instancia_assimetrica_dp_igual_forca_bruta_e_lb_valido) {
    for (std::uint64_t s = 1; s <= 60; ++s) {
        const Instance inst = asymmetric_instance(2 + static_cast<int>(s % 9), 5000 + s);
        const Cost opt = solve_brute_force(inst).cost;
        CHECK_EQ(solve_dp(inst).cost, opt);
        CHECK(mst_lower_bound(inst) <= opt);  // usa min(d(u,v), d(v,u)) na MST
    }
}

TEST(extremo_instancia_assimetrica_avaliacao_O1_exata) {
    Rng rng(51);
    const Instance inst = asymmetric_instance(12, 77);
    const Route r = random_route(inst.n, rng);
    const SubseqTable tab(inst, r);
    for (int i = 1; i < inst.n - 1; ++i)
        for (int j = i + 1; j < inst.n; ++j) {
            Route a = r, b = r;
            apply_swap(a, i, j);
            apply_two_opt(b, i, j);  // inversão percorre arestas no sentido contrário
            CHECK_EQ(eval_swap(inst, tab, i, j), latency(inst, a));
            CHECK_EQ(eval_two_opt(inst, tab, i, j), latency(inst, b));
        }
    for (int k = 1; k <= 3; ++k)
        for (int i = 1; i + k - 1 < inst.n; ++i)
            for (int p = 1; p <= inst.n; ++p) {
                if (p >= i && p <= i + k) continue;
                Route c = r;
                apply_or_opt(c, i, k, p);
                CHECK_EQ(eval_or_opt(inst, tab, i, k, p), latency(inst, c));
            }
}

TEST(extremo_pontos_coincidentes_custo_zero) {
    std::vector<std::vector<Cost>> zeros(8, std::vector<Cost>(8, 0));
    const Instance inst = from_matrix(zeros, "zeros");
    for (const Algorithm& a : all_algorithms()) {
        const Result r = a.run(inst, {});
        CHECK(is_valid_route(inst, r.route));
        CHECK_EQ(r.cost, 0);
    }
    CHECK_EQ(mst_lower_bound(inst), 0);
}

TEST(extremo_tsplib_truncado_ou_malformado_lanca_excecao) {
    const std::string p = "test_malformado.tsp";
    write_file(p, "NAME : t\nDIMENSION : 5\nEDGE_WEIGHT_TYPE : EUC_2D\nNODE_COORD_SECTION\n1 0 0\n2 1 0\nEOF\n");
    CHECK(load_throws(p));  // 2 de 5 coordenadas
    write_file(p, "NAME : t\nDIMENSION : 3\nEDGE_WEIGHT_TYPE : EXPLICIT\nEDGE_WEIGHT_FORMAT : FULL_MATRIX\n"
                  "EDGE_WEIGHT_SECTION\n0 1 2\n1 0\nEOF\n");
    CHECK(load_throws(p));  // matriz incompleta
    write_file(p, "NAME : t\nEDGE_WEIGHT_TYPE : EUC_2D\nNODE_COORD_SECTION\n1 0 0\nEOF\n");
    CHECK(load_throws(p));  // sem DIMENSION
    write_file(p, "NAME : t\nDIMENSION : 3\nEDGE_WEIGHT_TYPE : GEO\nNODE_COORD_SECTION\n1 0 0\n2 0 1\n3 1 1\nEOF\n");
    CHECK(load_throws(p));  // tipo não suportado
    CHECK(load_throws("arquivo_que_nao_existe.tsp"));
    std::remove(p.c_str());
}

TEST(tsplib_matriz_explicita_e_lida_corretamente) {
    const std::string p = "test_explicita.tsp";
    write_file(p, "NAME : reta\nDIMENSION : 4\nEDGE_WEIGHT_TYPE : EXPLICIT\nEDGE_WEIGHT_FORMAT : FULL_MATRIX\n"
                  "EDGE_WEIGHT_SECTION\n0 1 2 3\n1 0 1 2\n2 1 0 1\n3 2 1 0\nEOF\n");
    const Instance inst = load_tsplib(p);
    std::remove(p.c_str());
    CHECK_EQ(inst.n, 4);
    CHECK_EQ(inst.d(0, 3), 3);
    CHECK_EQ(solve_dp(inst).cost, 6);
}

TEST(registro_de_algoritmos_completo_e_consistente) {
    CHECK_EQ(all_algorithms().size(), std::size_t{5});
    CHECK(find_algorithm("inexistente") == nullptr);
    const Instance inst = make_random_euclidean(9, 42);
    const Cost opt = solve_dp(inst).cost;
    for (const Algorithm& a : all_algorithms()) {
        const Result r = run_algorithm(a.name, inst);
        CHECK(is_valid_route(inst, r.route));
        CHECK_EQ(r.cost, latency(inst, r.route));
        CHECK(r.cost >= opt);
        if (a.exact) CHECK_EQ(r.cost, opt);
    }
}

// ---------------------------------------------------------------------------
int main(int argc, char** argv) { return testing::run_all(argc > 1 ? argv[1] : ""); }
