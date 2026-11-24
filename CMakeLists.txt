cmake_minimum_required(VERSION 3.14)
project(MinimumLatencyProblem LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Configuração geral
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de build" FORCE)
endif()

# Release com -O2 (mesma flag usada nos benchmarks do relatório)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# Biblioteca principal
# ---------------------------------------------------------------------------
add_library(mlp
  src/algorithms.cpp
  src/instance.cpp
  src/solution.cpp
  src/exact.cpp
  src/construction.cpp
  src/subsequence.cpp
  src/local_search.cpp
  src/gils_rvnd.cpp
)
target_include_directories(mlp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# ---------------------------------------------------------------------------
# Executáveis
# ---------------------------------------------------------------------------
add_executable(mlp_solver apps/mlp_solver.cpp)
target_link_libraries(mlp_solver PRIVATE mlp)

add_executable(mlp_gen apps/mlp_gen.cpp)
target_link_libraries(mlp_gen PRIVATE mlp)

add_executable(mlp_bench apps/mlp_bench.cpp)
target_link_libraries(mlp_bench PRIVATE mlp)

# ---------------------------------------------------------------------------
# Testes (CTest)
# ---------------------------------------------------------------------------
enable_testing()
add_executable(mlp_tests tests/test_main.cpp)
target_link_libraries(mlp_tests PRIVATE mlp)
target_include_directories(mlp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME mlp_tests COMMAND mlp_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
