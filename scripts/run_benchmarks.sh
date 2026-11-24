#!/usr/bin/env bash
# =============================================================================
# run_benchmarks.sh — Compila, testa e roda TODOS os experimentos do relatório
# -----------------------------------------------------------------------------
# Uso:   ./scripts/run_benchmarks.sh          (completo: ~30–40 min num notebook i5)
#        ./scripts/run_benchmarks.sh --quick  (versão reduzida, ~15 s)
# Saída: results/*.csv (exact, quality, scale, tradeoff, perturb), results/environment.txt, results/figures/*.png,
#        results/summary.md
# =============================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build"
OUT="$ROOT/results"
mkdir -p "$OUT/figures"

echo "==> Compilando (Release, -O2)"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build "$BUILD" -j

echo "==> Testes de corretude"
(cd "$BUILD" && ctest --output-on-failure)

echo "==> Registrando ambiente de execução"
{
  echo "data: $(date -Iseconds)"
  echo "so: $(uname -srm)"
  echo "cpu: $( (lscpu 2>/dev/null | grep 'Model name' | sed 's/Model name:[[:space:]]*//') || sysctl -n machdep.cpu.brand_string 2>/dev/null || echo desconhecida)"
  echo "nucleos: $(nproc 2>/dev/null || sysctl -n hw.ncpu)"
  echo "compilador: $(${CXX:-c++} --version | head -n1)"
  echo "flags: -O2 -std=c++17 (CMAKE_BUILD_TYPE=Release)"
} > "$OUT/environment.txt"
cat "$OUT/environment.txt"

B="$BUILD/mlp_bench"
if [[ "${1:-}" == "--quick" ]]; then
  "$B" exact    --out "$OUT/exact.csv"    --mmin 3 --mmax 14 --brute-max 9 --instances 2
  "$B" quality  --out "$OUT/quality.csv"  --sizes 8,10,12 --instances 3
  "$B" scale    --out "$OUT/scale.csv"    --sizes 20,50,100 --instances 2 --seeds 2
  "$B" tradeoff --out "$OUT/tradeoff.csv" --n 50 --instances 2 --seeds 2 --iils 1,5,25,50
  "$B" perturb  --out "$OUT/perturb.csv"  --sizes 10,20,30 --instances 2 --trials 50
else
  "$B" exact    --out "$OUT/exact.csv"    --mmin 3 --mmax 20 --brute-max 11 --instances 3
  "$B" quality  --out "$OUT/quality.csv"  --sizes 8,10,12,14,16,18 --instances 10
  "$B" scale    --out "$OUT/scale.csv"    --sizes 20,50,100,200 --instances 5 --seeds 3
  "$B" tradeoff --out "$OUT/tradeoff.csv" --n 100 --instances 5 --seeds 3 --iils 1,2,5,10,25,50,100
  "$B" perturb  --out "$OUT/perturb.csv"  --sizes 10,20,30,60,100 --instances 5 --trials 100
fi

echo "==> Gerando gráficos e tabelas"
python3 "$ROOT/scripts/plot_results.py" "$OUT"
echo "==> Pronto: veja $OUT/summary.md e $OUT/figures/"
