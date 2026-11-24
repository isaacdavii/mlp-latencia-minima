// =============================================================================
// instance.hpp — Representação de uma instância do Minimum Latency Problem
// -----------------------------------------------------------------------------
// Uma instância é um grafo completo com n vértices (0 = depósito, 1..n-1 =
// clientes) e uma matriz de distâncias inteiras d(i, j).
//
// Usamos custos inteiros (int64) — como no TSPLIB, distâncias euclidianas são
// arredondadas para o inteiro mais próximo (nint). Isso torna as comparações
// exatas (sem tolerância de ponto flutuante), o que simplifica os testes de
// corretude (ex.: DP == força bruta).
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mlp {

using Cost = std::int64_t;

struct Instance {
    std::string name;
    int n = 0;                 ///< nº de vértices, incluindo o depósito (vértice 0)
    std::vector<Cost> dist;    ///< matriz n×n em ordem row-major
    std::vector<double> x, y;  ///< coordenadas (vazias se a instância for explícita)

    /// Distância d(i, j). Acesso O(1) a um vetor contíguo (cache-friendly).
    Cost d(int i, int j) const { return dist[static_cast<std::size_t>(i) * n + j]; }

    /// Número de clientes (vértices além do depósito).
    int customers() const { return n - 1; }
};

/// Gera uma instância euclidiana aleatória: n pontos com coordenadas inteiras
/// uniformes em [0, side] × [0, side]; distância = nint(euclidiana).
/// Mesma (n, seed) ⇒ mesma instância em qualquer plataforma.
Instance make_random_euclidean(int n, std::uint64_t seed, int side = 1000);

/// Cria uma instância a partir de uma matriz de distâncias explícita.
Instance from_matrix(const std::vector<std::vector<Cost>>& m, const std::string& name = "matrix");

/// Lê um arquivo no formato TSPLIB. Suporta:
///   EDGE_WEIGHT_TYPE: EUC_2D  (NODE_COORD_SECTION)
///   EDGE_WEIGHT_TYPE: EXPLICIT com EDGE_WEIGHT_FORMAT: FULL_MATRIX
/// O primeiro nó do arquivo é tratado como depósito.
Instance load_tsplib(const std::string& path);

/// Salva uma instância euclidiana no formato TSPLIB EUC_2D.
void save_tsplib(const Instance& inst, const std::string& path);

}  // namespace mlp
