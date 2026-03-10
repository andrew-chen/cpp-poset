# Makefile for cpp-poset

CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pthread
LDFLAGS = -pthread

# Targets
LIB = poset.o
EXAMPLE = example
BENCHMARK = benchmark

.PHONY: all lib example bench clean

# Default: build everything
all: lib example bench

# Library object
lib: $(LIB)

$(LIB): poset.cpp poset.h
	$(CXX) $(CXXFLAGS) -c poset.cpp -o $(LIB)

# Example program
example: $(EXAMPLE)

$(EXAMPLE): example.cpp $(LIB)
	$(CXX) $(CXXFLAGS) example.cpp $(LIB) $(LDFLAGS) -o $(EXAMPLE)

# Benchmark harness
bench: $(BENCHMARK)

$(BENCHMARK): test_harness.cpp $(LIB)
	$(CXX) $(CXXFLAGS) test_harness.cpp $(LIB) $(LDFLAGS) -o $(BENCHMARK)

# Clean build artifacts
clean:
	rm -f $(LIB) $(EXAMPLE) $(BENCHMARK) poset_benchmark_results.csv

# Run example
run-example: $(EXAMPLE)
	./$(EXAMPLE)

# Run benchmark (generates CSV)
run-benchmark: $(BENCHMARK)
	./$(BENCHMARK)
	@echo "Benchmark complete. Results in poset_benchmark_results.csv"
	@echo "Run 'python3 poset_analysis.py' to generate visualizations."
