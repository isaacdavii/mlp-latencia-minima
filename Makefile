# Alternativa ao CMake (Linux/macOS/MinGW): make && make test
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build-make/%.o)
BIN      := build-make

all: $(BIN)/mlp_solver $(BIN)/mlp_gen $(BIN)/mlp_bench $(BIN)/mlp_tests

$(BIN)/%.o: src/%.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BIN)/mlp_%: apps/mlp_%.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BIN)/mlp_tests: tests/test_main.cpp $(OBJ)
	$(CXX) $(CXXFLAGS) -Itests $^ -o $@

$(BIN):
	mkdir -p $(BIN)

test: $(BIN)/mlp_tests
	cd $(BIN) && ./mlp_tests

clean:
	rm -rf $(BIN)

.PHONY: all test clean
