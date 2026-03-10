// -----------------------------------------------------------------------------
// Test Harness for Dynamic Implicit Strict Poset
// -----------------------------------------------------------------------------
// Save this in the same file as the Poset implementation or include the header.

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <iomanip>

#include "poset.h"

void run_benchmark_suite(const std::string& filename) {
    std::ofstream csv_file(filename);
    
    // CSV Header
    csv_file << "Experiment,NumNodes,NumEdges,Operation,TimePerOp_NS,ResultType\n";
    
    std::cout << "Starting Benchmark Suite...\n";
    std::cout << "Results will be written to: " << filename << "\n\n";

    // Configuration
    std::vector<int> node_counts = {100, 500, 1000, 2000, 5000};
    int queries_per_batch = 1000;

    // Random Number Generation
    std::mt19937 rng(42); // Fixed seed for reproducibility

    for (int V : node_counts) {
        std::cout << "Running tests for V = " << V << "...\n";

        auto poset = std::make_shared<Poset>();
        std::vector<NodeRef> nodes;
        nodes.reserve(V);

        // 1. Node Creation Phase
        for (int i = 0; i < V; ++i) {
            nodes.push_back(poset->get_node("Node_" + std::to_string(i)));
        }

        // 2. Insertion Phase (Building the Graph)
        // We will try to add edges until we reach ~2*V edges (Sparse) to ~5*V (Medium)
        int target_edges = V * 5; 
        int edges_added = 0;
        int attempts = 0;

        std::uniform_int_distribution<int> dist(0, V - 1);

        while (edges_added < target_edges && attempts < target_edges * 2) {
            int idx_a = dist(rng);
            int idx_b = dist(rng);

            if (idx_a == idx_b) continue;

            NodeRef a = nodes[idx_a];
            NodeRef b = nodes[idx_b];

            // Measure Assert Time
            auto start = std::chrono::high_resolution_clock::now();
            CandidateSet result = poset->assert_less_than(a, b);
            auto end = std::chrono::high_resolution_clock::now();

            double duration_ns = std::chrono::duration<double, std::nano>(end - start).count();

            bool cycle_detected = !result.empty();

            // Log output every 100 successful edges to keep CSV size manageable 
            // but granular enough to see degradation as E grows.
            if (edges_added % 100 == 0) {
                 csv_file << "Insert," << V << "," << edges_added << ",Assert," 
                          << duration_ns << "," << (cycle_detected ? "Cycle" : "Success") << "\n";
            }

            if (!cycle_detected) {
                edges_added++;
            }
            attempts++;
        }

        // 3. Query Phase (Read Heavy)
        // Measure random queries on the constructed graph
        long long total_query_time = 0;
        
        for (int q = 0; q < queries_per_batch; ++q) {
            int idx_a = dist(rng);
            int idx_b = dist(rng);

            auto start = std::chrono::high_resolution_clock::now();
            poset->is_less_than(nodes[idx_a], nodes[idx_b]);
            auto end = std::chrono::high_resolution_clock::now();

            total_query_time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }

        double avg_query_time = static_cast<double>(total_query_time) / queries_per_batch;
        
        csv_file << "Query," << V << "," << edges_added << ",IsLessThan," 
                 << avg_query_time << ",Average\n";
    }

    csv_file.close();
    std::cout << "Benchmark complete.\n";
}

int main() {
    // Run the benchmark
    try {
        run_benchmark_suite("poset_benchmark_results.csv");
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
