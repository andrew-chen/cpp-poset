// example.cpp - Demonstrates cpp-poset usage for build dependency tracking
#include "poset.h"
#include <iostream>
#include <string>

int main() {
    // Create a poset to track build dependencies
    auto deps = std::make_shared<Poset>();
    
    std::cout << "=== Build Dependency Example ===" << std::endl << std::endl;
    
    // Define components
    auto libCore = deps->get_node("libCore");
    auto libUtils = deps->get_node("libUtils");
    auto libNetwork = deps->get_node("libNetwork");
    auto app = deps->get_node("app");
    
    std::cout << "Components: libCore, libUtils, libNetwork, app" << std::endl << std::endl;
    
    // Define dependency constraints
    std::cout << "Setting up dependencies:" << std::endl;
    
    // libUtils depends on libCore
    auto result1 = deps->assert_less_than(libCore, libUtils);
    if (result1.empty()) {
        std::cout << "  ✓ libCore must build before libUtils" << std::endl;
    }
    
    // libNetwork depends on libCore
    auto result2 = deps->assert_less_than(libCore, libNetwork);
    if (result2.empty()) {
        std::cout << "  ✓ libCore must build before libNetwork" << std::endl;
    }
    
    // app depends on libUtils
    auto result3 = deps->assert_less_than(libUtils, app);
    if (result3.empty()) {
        std::cout << "  ✓ libUtils must build before app" << std::endl;
    }
    
    // app depends on libNetwork
    auto result4 = deps->assert_less_than(libNetwork, app);
    if (result4.empty()) {
        std::cout << "  ✓ libNetwork must build before app" << std::endl;
    }
    
    std::cout << std::endl;
    
    // Query transitive dependencies
    std::cout << "Checking transitive dependencies:" << std::endl;
    
    if (deps->is_less_than(libCore, app)) {
        std::cout << "  ✓ libCore must build before app (transitive)" << std::endl;
    }
    
    if (!deps->is_less_than(libUtils, libNetwork)) {
        std::cout << "  ✓ libUtils and libNetwork can build in parallel" << std::endl;
    }
    
    std::cout << std::endl;
    
    // Attempt to create a circular dependency (will fail)
    std::cout << "Attempting to add circular dependency:" << std::endl;
    std::cout << "  Trying: app must build before libCore (creates cycle)" << std::endl;
    
    auto conflicts = deps->assert_less_than(app, libCore);
    
    if (!conflicts.empty()) {
        std::cout << "  ✗ Cycle detected! Cannot add this constraint." << std::endl;
        std::cout << "  Conflict proof contains " << conflicts.size() 
                  << " edge(s) forming the cycle." << std::endl;
        
        std::cout << std::endl << "  Cycle edges:" << std::endl;
        for (const auto& edge : conflicts) {
            std::cout << "    - Part of conflicting path" << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "=== Example Complete ===" << std::endl;
    
    return 0;
}
