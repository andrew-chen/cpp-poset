#include "poset.h"
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <stdexcept>
#include <algorithm>

// -----------------------------------------------------------------------------
// NodeRef Implementation
// -----------------------------------------------------------------------------

bool NodeRef::belongs_to(const std::shared_ptr<Poset>& p) const {
    return !poset.expired() && poset.lock() == p;
}

// -----------------------------------------------------------------------------
// CandidateSet Implementation
// -----------------------------------------------------------------------------

CandidateSet::CandidateSet(std::shared_ptr<Poset> p) : poset_ref(p) {}

void CandidateSet::add_conflict(const EdgeTriple& edge) {
    conflicts.push_back(edge);
}

CandidateSet::Iterator CandidateSet::begin() { 
    return Iterator(conflicts.begin(), poset_ref); 
}

CandidateSet::Iterator CandidateSet::end() { 
    return Iterator(conflicts.end(), poset_ref); 
}

bool CandidateSet::empty() const { 
    return conflicts.empty(); 
}

size_t CandidateSet::size() const { 
    return conflicts.size(); 
}

ConflictItem CandidateSet::Iterator::operator*() {
    NodeRef u(current->source, poset_ref);
    NodeRef v(current->target, poset_ref);
    return {u, v, current->timestamp};
}

// -----------------------------------------------------------------------------
// Poset Implementation
// -----------------------------------------------------------------------------

Poset::Poset() : next_id(1) {}

NodeRef Poset::get_node(const std::string& name) {
    std::unique_lock lock(mutex_); // Writer lock
    if (name_to_id.find(name) == name_to_id.end()) {
        name_to_id[name] = next_id++;
    }
    return NodeRef(name_to_id[name], shared_from_this());
}

uint64_t Poset::get_timestamp() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

void Poset::validate_refs(const NodeRef& a, const NodeRef& b) const {
    auto self = const_cast<Poset*>(this)->shared_from_this();
    if (!a.belongs_to(self) || !b.belongs_to(self)) {
        throw std::invalid_argument("NodeRef does not belong to this Poset.");
    }
}

// -----------------------------------------------------------------------------
// TEST Operation
// -----------------------------------------------------------------------------

bool Poset::is_less_than(const NodeRef& a, const NodeRef& b) {
    validate_refs(a, b);
    if (a.id == b.id) return false;

    std::shared_lock lock(mutex_); // Reader lock

    return is_less_than_internal(a.id, b.id);
}

bool Poset::is_less_than_internal(uint64_t u, uint64_t v) {
    // 1. Explicit Check
    EdgeTriple query = {u, v, 0, Direction::OUTGOING};
    auto it = edge_store.lower_bound(query);
    if (it != edge_store.end() && it->source == u && it->target == v && it->direction == Direction::OUTGOING) {
        return true;
    }

    // 2. Implicit Check (Arena-based DFS)
    std::vector<TraceNode> arena; 
    std::unordered_map<uint64_t, bool> visited; 
    
    return find_path(u, v, arena, visited);
}

// -----------------------------------------------------------------------------
// ASSERT Operation
// -----------------------------------------------------------------------------

CandidateSet Poset::assert_less_than(const NodeRef& a, const NodeRef& b) {
    validate_refs(a, b);
    if (a.id == b.id) throw std::invalid_argument("Cannot assert strict order on identical nodes.");

    std::unique_lock lock(mutex_); // Writer lock (Exclusive)

    // 1. Check if relation already exists
    if (is_less_than_internal(a.id, b.id)) {
        return CandidateSet(shared_from_this()); // Already consistent
    }

    // 2. Check for Conflict (Is b < a ?)
    // If b < a is true, then adding a < b creates a cycle.
    std::vector<TraceNode> arena;
    std::unordered_map<uint64_t, bool> visited;
    
    if (find_path(b.id, a.id, arena, visited)) {
        // Conflict found! Trace back to find the cycle
        return construct_candidate_set(arena, b.id, a.id);
    }

    // 3. No Conflict: Insert Edge (Dual Insertion)
    uint64_t now = get_timestamp();
    edge_store.insert({a.id, b.id, now, Direction::OUTGOING});
    edge_store.insert({b.id, a.id, now, Direction::INCOMING}); // For reverse lookups

    return CandidateSet(shared_from_this()); // Success (empty set)
}

// -----------------------------------------------------------------------------
// Core Algorithms: Recursion & Arena
// -----------------------------------------------------------------------------

bool Poset::find_path(uint64_t start, uint64_t target, 
                      std::vector<TraceNode>& arena, 
                      std::unordered_map<uint64_t, bool>& visited) {
    
    // Initialize: Push the root
    visited[start] = true;
    arena.push_back({start, (size_t)-1, 0}); 

    return recursive_search_step(0, target, arena, visited);
}

bool Poset::recursive_search_step(size_t current_index, uint64_t target, 
                                  std::vector<TraceNode>& arena, 
                                  std::unordered_map<uint64_t, bool>& visited) {
    
    uint64_t u = arena[current_index].node_id;

    // Iterate through OUTGOING edges of 'u'
    auto it = edge_store.lower_bound({u, 0, 0, Direction::OUTGOING});

    while (it != edge_store.end() && it->source == u) {
        if (it->direction == Direction::OUTGOING) {
            uint64_t v = it->target;
            uint64_t ts = it->timestamp;

            if (v == target) {
                // SUCCESS: Record the final step and return immediately
                arena.push_back({v, current_index, ts});
                return true;
            }

            if (!visited[v]) {
                visited[v] = true;
                
                // BUMP ALLOCATION: Push 'v' to the end of the arena.
                size_t next_index = arena.size();
                arena.push_back({v, current_index, ts});

                // Recurse
                if (recursive_search_step(next_index, target, arena, visited)) {
                    return true; 
                }
            }
        }
        ++it;
    }

    return false; 
}

CandidateSet Poset::construct_candidate_set(const std::vector<TraceNode>& arena, 
                                            uint64_t start, uint64_t end) {
    CandidateSet candidates(shared_from_this());
    
    if (arena.empty() || arena.back().node_id != end) {
        return candidates; 
    }

    // Start at the end of the arena (the target node)
    size_t current_idx = arena.size() - 1;

    while (true) {
        const TraceNode& curr = arena[current_idx];

        // Stop if we hit the root (which has parent -1)
        if (curr.parent_index == (size_t)-1) {
            break;
        }

        // Identify the parent node to reconstruct the edge
        const TraceNode& parent = arena[curr.parent_index];

        // The edge is Parent -> Curr
        EdgeTriple conflict_edge;
        conflict_edge.source = parent.node_id;
        conflict_edge.target = curr.node_id;
        conflict_edge.timestamp = curr.timestamp;
        conflict_edge.direction = Direction::OUTGOING;

        candidates.add_conflict(conflict_edge);

        // Move back
        current_idx = curr.parent_index;
    }
    return candidates;
}
