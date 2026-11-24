// =============================================================================
// instance.cpp — Geração, leitura e escrita de instâncias
// =============================================================================
#include "mlp/instance.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "mlp/rng.hpp"

namespace mlp {

namespace {

/// Distância euclidiana arredondada (função nint do TSPLIB).
Cost euclid_nint(double x1, double y1, double x2, double y2) {
    const double dx = x1 - x2, dy = y1 - y2;
    return static_cast<Cost>(std::sqrt(dx * dx + dy * dy) + 0.5);
}

/// Preenche a matriz de distâncias a partir das coordenadas.
void build_euclidean_matrix(Instance& inst) {
    inst.dist.assign(static_cast<std::size_t>(inst.n) * inst.n, 0);
    for (int i = 0; i < inst.n; ++i)
        for (int j = 0; j < inst.n; ++j)
            inst.dist[static_cast<std::size_t>(i) * inst.n + j] =
                euclid_nint(inst.x[i], inst.y[i], inst.x[j], inst.y[j]);
}

std::string trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

}  // namespace

Instance make_random_euclidean(int n, std::uint64_t seed, int side) {
    if (n < 1) throw std::invalid_argument("n deve ser >= 1");
    Instance inst;
    inst.name = "rand_n" + std::to_string(n) + "_s" + std::to_string(seed);
    inst.n = n;
    Rng rng(seed);
    inst.x.resize(n);
    inst.y.resize(n);
    for (int i = 0; i < n; ++i) {
        inst.x[i] = static_cast<double>(rng.uniform_int(0, side));
        inst.y[i] = static_cast<double>(rng.uniform_int(0, side));
    }
    build_euclidean_matrix(inst);
    return inst;
}

Instance from_matrix(const std::vector<std::vector<Cost>>& m, const std::string& name) {
    Instance inst;
    inst.name = name;
    inst.n = static_cast<int>(m.size());
    inst.dist.resize(static_cast<std::size_t>(inst.n) * inst.n);
    for (int i = 0; i < inst.n; ++i) {
        if (static_cast<int>(m[i].size()) != inst.n)
            throw std::invalid_argument("matriz de distâncias não é quadrada");
        for (int j = 0; j < inst.n; ++j) inst.dist[static_cast<std::size_t>(i) * inst.n + j] = m[i][j];
    }
    return inst;
}

Instance load_tsplib(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("não foi possível abrir " + path);

    Instance inst;
    std::string type, format, line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line == "EOF") break;

        const auto colon = line.find(':');
        if (colon != std::string::npos) {  // linha de cabeçalho "CHAVE : valor"
            const std::string key = trim(line.substr(0, colon));
            const std::string val = trim(line.substr(colon + 1));
            if (key == "NAME") inst.name = val;
            else if (key == "DIMENSION") inst.n = std::stoi(val);
            else if (key == "EDGE_WEIGHT_TYPE") type = val;
            else if (key == "EDGE_WEIGHT_FORMAT") format = val;
            continue;
        }

        if ((line == "NODE_COORD_SECTION" || line == "EDGE_WEIGHT_SECTION") && inst.n <= 0)
            throw std::runtime_error("DIMENSION ausente antes da seção de dados em " + path);
        if (line == "NODE_COORD_SECTION") {
            if (type != "EUC_2D") throw std::runtime_error("apenas EUC_2D é suportado para coordenadas");
            inst.x.resize(inst.n);
            inst.y.resize(inst.n);
            for (int i = 0; i < inst.n; ++i) {
                int id;
                if (!(in >> id >> inst.x[i] >> inst.y[i]))
                    throw std::runtime_error("NODE_COORD_SECTION incompleta: esperados " +
                                             std::to_string(inst.n) + " nós em " + path);
            }
            build_euclidean_matrix(inst);
        } else if (line == "EDGE_WEIGHT_SECTION") {
            if (format != "FULL_MATRIX") throw std::runtime_error("apenas FULL_MATRIX é suportado");
            inst.dist.resize(static_cast<std::size_t>(inst.n) * inst.n);
            for (auto& v : inst.dist)
                if (!(in >> v)) throw std::runtime_error("EDGE_WEIGHT_SECTION incompleta em " + path);
        }
    }
    if (inst.n <= 0 || inst.dist.size() != static_cast<std::size_t>(inst.n) * inst.n)
        throw std::runtime_error("arquivo TSPLIB incompleto: " + path);
    return inst;
}

void save_tsplib(const Instance& inst, const std::string& path) {
    if (inst.x.size() != static_cast<std::size_t>(inst.n))
        throw std::invalid_argument("save_tsplib requer instância com coordenadas");
    std::ofstream out(path);
    if (!out) throw std::runtime_error("não foi possível escrever " + path);
    out << "NAME : " << inst.name << "\n"
        << "COMMENT : Minimum Latency Problem - no 1 e o deposito\n"
        << "TYPE : TSP\n"
        << "DIMENSION : " << inst.n << "\n"
        << "EDGE_WEIGHT_TYPE : EUC_2D\n"
        << "NODE_COORD_SECTION\n";
    for (int i = 0; i < inst.n; ++i)
        out << (i + 1) << " " << static_cast<long long>(inst.x[i]) << " " << static_cast<long long>(inst.y[i]) << "\n";
    out << "EOF\n";
}

}  // namespace mlp
