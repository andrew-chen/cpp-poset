#ifndef POSET_H
#define POSET_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <memory>
#include <shared_mutex>
#include <cstdint>
#include <iterator>

// Forward declarations
class Poset;
class CandidateSet;

// -----------------------------------------------------------------------------
// Internal Structures (Exposed in header for Member usage)
// -----------------------------------------------------------------------------

enum class Direction : uint8_t { OUTGOING = 0, INCOMING = 1 };

struct EdgeTriple {
    uint64_t source;
    uint64_t target;
    uint64_t timestamp; 
    Direction direction;

    // Sort primarily by source to group outgoing edges together
    bool operator<(const EdgeTriple& other) const {
        if (source != other.source) return source < other.source;
        if (target != other.target) return target < other.target;
        return timestamp < other.timestamp;
    }
};

struct TraceNode {
    uint64_t node_id;
    size_t parent_index; // Index in the Arena vector
    uint64_t timestamp;
};

// -----------------------------------------------------------------------------
// Public Interface Types
// -----------------------------------------------------------------------------

// Opaque reference to a node, safe for user handling
class NodeRef {
public:
    friend class Poset;
    friend class CandidateSet;

    NodeRef() : id(0), poset() {}

    // Check if this reference is valid and points to a specific poset
    bool belongs_to(const std::shared_ptr<Poset>& p) const;
    uint64_t get_id() const { return id; }

private:
    uint64_t id;
    std::weak_ptr<Poset> poset;

    NodeRef(uint64_t _id, std::shared_ptr<Poset> _p) : id(_id), poset(_p) {}
};

// Item returned when iterating over conflicts
struct ConflictItem {
    NodeRef from;
    NodeRef to;
    uint64_t timestamp;
};

// -----------------------------------------------------------------------------
// CandidateSet Class
// -----------------------------------------------------------------------------
class CandidateSet {
public:
    friend class Poset;

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = ConflictItem;
        using difference_type = std::ptrdiff_t;
        using pointer = ConflictItem*;
        using reference = ConflictItem&;

        Iterator(std::vector<EdgeTriple>::const_iterator ptr, std::shared_ptr<Poset> p) 
            : current(ptr), poset_ref(p) {}

        ConflictItem operator*();
        Iterator& operator++() { ++current; return *this; }
        bool operator!=(const Iterator& other) const { return current != other.current; }

    private:
        std::vector<EdgeTriple>::const_iterator current;
        std::shared_ptr<Poset> poset_ref;
    };

    Iterator begin();
    Iterator end();
    bool empty() const;
    size_t size() const;

private:
    std::vector<EdgeTriple> conflicts;
    std::shared_ptr<Poset> poset_ref;

    CandidateSet(std::shared_ptr<Poset> p);
    void add_conflict(const EdgeTriple& edge);
};

// -----------------------------------------------------------------------------
// Poset Class
// -----------------------------------------------------------------------------
class Poset : public std::enable_shared_from_this<Poset> {
public:
    friend class NodeRef;
    friend class CandidateSet; // Needs access to EdgeStore/Internals if expanding logic

    Poset();

    // Register or retrieve a node by name
    NodeRef get_node(const std::string& name);

    // TEST Operation: Returns true if a < b
    bool is_less_than(const NodeRef& a, const NodeRef& b);

    // ASSERT Operation: Tries to add a < b
    // Returns a CandidateSet. If empty, assertion succeeded. 
    // If not, assertion failed due to cycles, and the set contains the conflicting edges.
    CandidateSet assert_less_than(const NodeRef& a, const NodeRef& b);

    // Friend accessor for the CandidateSet iterator (needs access to internals in some designs)
    // or simply for the internal logic.
    friend class CandidateSet::Iterator;

private:
    // --- Data ---
    std::multiset<EdgeTriple> edge_store; 
    std::map<std::string, uint64_t> name_to_id;
    uint64_t next_id;
    
    mutable std::shared_mutex mutex_; 

    // --- Helpers ---
    uint64_t get_timestamp();
    void validate_refs(const NodeRef& a, const NodeRef& b) const;

    // Internal version of is_less_than without locking
    bool is_less_than_internal(uint64_t u, uint64_t v);

    // Core Algorithm: Arena-based DFS
    // Returns true if a path from 'u' (at current_index in arena) to 'target' exists.
    bool recursive_search_step(size_t current_index, uint64_t target, 
                               std::vector<TraceNode>& arena, 
                               std::unordered_map<uint64_t, bool>& visited);
                               
    // Wrapper to start search
    bool find_path(uint64_t start, uint64_t target, 
                   std::vector<TraceNode>& arena, 
                   std::unordered_map<uint64_t, bool>& visited);

    // Reconstruct the path from b -> ... -> a
    CandidateSet construct_candidate_set(const std::vector<TraceNode>& arena, 
                                         uint64_t start, uint64_t end);
};

#endif // POSET_H
