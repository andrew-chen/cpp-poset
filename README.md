# cpp-poset

A high-performance C++17 library for maintaining strict partial orders (posets) with fast transitive closure queries and cycle detection.

## What is cpp-poset?

cpp-poset efficiently maintains ordering constraints over thousands of items. When you assert `A < B < C`, the system automatically enforces transitivity (`A < C`) and detects violations (attempting `C < A` produces a cycle proof).

**Key features:**
- **Fast**: Sub-millisecond queries on graphs with 5000+ nodes, 25000+ edges
- **Correct**: Detects cycles and provides conflict proofs for debugging
- **Cache-aware**: Optimized data structures for modern CPU architectures
- **Thread-safe**: Concurrent reads with exclusive writes via shared_mutex
- **Zero dependencies**: Just C++17 standard library

## Use Cases

- **Build systems**: Dependency resolution, cycle detection in build graphs
- **Package managers**: Version constraints, dependency conflict resolution  
- **Temporal reasoning**: Event ordering, timeline construction, causality tracking
- **Databases**: Query optimization, foreign key constraint validation
- **Scheduling**: Task dependencies, critical path analysis
- **Version control**: Commit ancestry, merge conflict detection
- **Knowledge graphs**: Ontology reasoning, transitive relationship inference

## Example

```cpp
#include "poset.h"
#include <iostream>

int main() {
    auto poset = std::make_shared<Poset>();

    // Define ordering constraints
    auto A = poset->get_node("A");
    auto B = poset->get_node("B");
    auto C = poset->get_node("C");

    poset->assert_less_than(A, B);  // A < B
    poset->assert_less_than(B, C);  // B < C

    // Query: is A < C? (transitive)
    if (poset->is_less_than(A, C)) {
        std::cout << "A < C (transitive)" << std::endl;
    }

    // Detect cycles
    auto conflicts = poset->assert_less_than(C, A);  // Would create cycle
    if (!conflicts.empty()) {
        std::cout << "Cycle detected! Conflict edges:" << std::endl;
        for (const auto& edge : conflicts) {
            std::cout << "  Conflict in cycle" << std::endl;
        }
    }

    return 0;
}
```

## Building

Using the provided Makefile:

```bash
make           # Build everything (library, example, benchmark)
make lib       # Build library only
make example   # Build example program
make benchmark # Build benchmark harness
make clean     # Clean build artifacts
```

Or compile manually:

```bash
g++ -std=c++17 -O3 -pthread -c poset.cpp
g++ -std=c++17 -O3 -pthread example.cpp poset.o -o example
```

## Performance

Benchmarked on graphs with up to 5000 vertices and 25000 edges:

- **Query time** (is_less_than): ~500 μs average for V=5000, sub-millisecond
- **Insertion time** (assert_less_than, successful): ~600 μs average for V=5000, E=25000
- **Cycle detection**: Returns conflict proof when violations detected

Performance scales approximately linearly with graph size for typical queries. See `test_harness.cpp` and run `poset_analysis.py` for detailed benchmark visualizations.

## API

### Core Operations

**Creating a poset:**
```cpp
auto poset = std::make_shared<Poset>();
```

**Getting or creating nodes:**
```cpp
NodeRef node = poset->get_node("node_name");
```

**TEST: Check if ordering holds**
```cpp
bool result = poset->is_less_than(nodeA, nodeB);
// Returns true if A < B (explicit or transitive)
```

**ASSERT: Add ordering constraint**
```cpp
CandidateSet conflicts = poset->assert_less_than(nodeA, nodeB);
if (conflicts.empty()) {
    // Success: constraint added (A < B now holds)
} else {
    // Failure: would create cycle
    // conflicts contains edges forming the cycle proof
    for (const auto& edge : conflicts) {
        // Examine conflict edges
    }
}
```

## Thread Safety

The implementation uses `std::shared_mutex` for thread safety:

- **Multiple readers**: Concurrent `is_less_than()` queries are safe
- **Single writer**: `assert_less_than()` acquires exclusive lock
- **Reader-writer exclusion**: Queries and assertions properly exclude each other

This enables high-concurrency read workloads while maintaining correctness during updates.

## Implementation Details

- **Algorithm**: Arena-based depth-first search with bump allocation for cache efficiency
- **Storage**: Dual-indexed edges (outgoing/incoming) in `std::multiset` for bidirectional traversal
- **Timestamps**: Nanosecond precision for deterministic edge ordering
- **Cycle detection**: Path reconstruction via parent pointers in search arena

## Status

Initial release: March 10, 2026

## License

Apache License 2.0 - See LICENSE file for details.

## Contact

Andrew Chen  
Minnesota State University Moorhead  
chenan@mnstate.edu

## Contributing

Contributions welcome! Please open issues or pull requests on GitHub.
