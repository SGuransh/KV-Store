#pragma once

#ifndef BTREENODE_HPP
#define BTREENODE_HPP

#include <cstdint>
#include <cstring>
#include "../BufferPool/Page.hpp"

// Maximum allowed tree height (internal levels + leaf level). Adjust if deeper trees are needed.
constexpr uint32_t MAX_TREE_HEIGHT = 8;

// Helper functions to calculate node capacities dynamically.
namespace BTreeCapacity {
    constexpr uint32_t getMaxLeafPairs() {
        return Page::PAGE_SIZE / (sizeof(int32_t) * 2);
    }

    constexpr uint32_t getMaxInternalKeys() {
        return Page::PAGE_SIZE / sizeof(int32_t);
    }
}

// Global constants for convenience.
constexpr uint32_t MAX_LEAF_PAIRS = BTreeCapacity::getMaxLeafPairs();
constexpr uint32_t MAX_INTERNAL_KEYS = BTreeCapacity::getMaxInternalKeys();
constexpr uint32_t MAX_INTERNAL_CHILDREN = MAX_INTERNAL_KEYS + 1;

/**
 * Metadata page - stored at page 0 of every SST file
 * Contains information about the B-Tree structure and bloom filter
 * Bloom filter is stored at the END of the SST file
 */
struct MetadataPage {    
    uint32_t magic;          // Magic number for file validation (0xB7EE0000)
    uint32_t rootPageId;     // Page ID of the root node
    uint32_t treeHeight;     // Height of the B-Tree (number of internal levels + 1 for leaves)
    uint32_t leafCount;      // Number of leaf pages 
    uint32_t lastLeafPairs;  // Number of pairs in the last leaf (1 to MAX_LEAF_PAIRS)
    int32_t minKey;          // Smallest key in the SST (for range filtering)
    int32_t maxKey;          // Largest key in the SST (for range filtering)
    
    // Per-level metadata (for fast search without counting nodes)
    // Level 0 = leaves, Level 1+ = internal nodes
    uint32_t nodesPerLevel[MAX_TREE_HEIGHT];     // Number of nodes at each level
    uint32_t lastNodeKeys[MAX_TREE_HEIGHT];      // Number of keys/pairs in last node at each level

    // Bloom filter metadata (filter stored at END of SST file)
    uint32_t bloom_bits;         // Total number of bits in bloom filter
    uint32_t bloom_bytes;        // Total bytes allocated for bloom filter
    uint32_t bloom_hash_count;   // Number of hash functions used (k)

    uint32_t leaf_start_page;   // Page ID where leaf nodes start
    
    static constexpr uint32_t MAGIC_NUMBER = 0xB7EE0000;
    
    MetadataPage() 
        : magic(MAGIC_NUMBER), rootPageId(0), treeHeight(0), 
          leafCount(0), lastLeafPairs(0), 
          minKey(0), maxKey(0),
          bloom_bits(0), bloom_bytes(0), bloom_hash_count(0) {
        std::memset(nodesPerLevel, 0, sizeof(nodesPerLevel));
        std::memset(lastNodeKeys, 0, sizeof(lastNodeKeys));
    }
    
    /**
     * Check if a page ID corresponds to a leaf page
     * @param pageId The page ID to check
     * @return true if it's a leaf page, false otherwise
     */
    bool isLeafPage(uint32_t pageId) const {
        return pageId >= 1 && pageId <= leafCount;
    }
    
    /**
     * Check if a page ID corresponds to an internal page
     * @param pageId The page ID to check
     * @return true if it's an internal page, false otherwise
     */
    bool isInternalPage(uint32_t pageId) const {
        return pageId > leafCount;
    }
    
    /**
     * Get the number of keys in a specific internal node
     * @param nodeIndexInLevel The index of the node within its level (0-based)
     * @param level The level (1 = first internal level above leaves, 2 = next level up, etc.)
     * @return Number of valid keys in that node
     */
    uint32_t getInternalNodeKeyCount(uint32_t nodeIndexInLevel, uint32_t level) const {
        if (level == 0 || level >= treeHeight) return 0;  // Invalid level
        
        uint32_t totalNodes = nodesPerLevel[level];
        bool isLastNode = (nodeIndexInLevel == totalNodes - 1);
        
        if (isLastNode) {
            // Last node at this level - may be partially filled
            uint32_t keysInLastNode = lastNodeKeys[level];
            return (keysInLastNode > 0) ? keysInLastNode : MAX_INTERNAL_KEYS;
        } else {
            // All other nodes are completely filled
            return MAX_INTERNAL_KEYS;
        }
    }
    
    /**
     * Get the number of pairs in a specific leaf node
     * @param leafIndex The index of the leaf (0-based, 0 = first leaf)
     * @return Number of valid key-value pairs in that leaf
     */
    uint32_t getLeafNodePairCount(uint32_t leafIndex) const {
        bool isLastLeaf = (leafIndex == leafCount - 1);
        
        if (isLastLeaf) {
            return lastLeafPairs;
        } else {
            return MAX_LEAF_PAIRS;
        }
    }
    
    /**
     * Serialize metadata to a Page
     */
    void serialize(Page& page) const {
        char* data = page.getData();
        std::memcpy(data, this, sizeof(MetadataPage));
    }
    
    /**
     * Deserialize metadata from a Page
     */
    bool deserialize(const Page& page) {
        const char* data = page.getData();
        std::memcpy(this, data, sizeof(MetadataPage));
        return magic == MAGIC_NUMBER;  // Validate magic number
    }
};

/**
 * Calculate maximum capacities based on page size (dynamic based on Page::PAGE_SIZE)
 * 
 * OPTIMIZED LEAF NODE (NO metadata fields):
 * Page size: PAGE_SIZE bytes (e.g., 4096 for production)
 * No overhead: type, numPairs, nextLeaf all removed
 * Each pair: key(4) + value(4) = 8 bytes
 * Available: PAGE_SIZE bytes (full page)
 * Max pairs: PAGE_SIZE / 8
 * 
 * Example (4096 bytes): 4096 / 8 = 512 pairs
 * 
 * INTERNAL NODE CALCULATION (no metadata, just keys):
 * Fixed overhead: 0 bytes (no metadata stored)
 * Each key: 4 bytes
 * NO children array (sequential: calculated from position)
 * Available: PAGE_SIZE bytes (full page for keys)
 * Max keys: PAGE_SIZE / 4
 * Max children: Max keys + 1
 * 
 * Example (4096 bytes): 4096 / 4 = 1024 keys, 1025 children
 */

/**
 * Leaf node - contains actual key-value pairs (OPTIMIZED - No metadata)
 * 
 * OPTIMIZATION 1: Removed type, numPairs, and nextLeaf fields
 * - type: Determined by page ID range (pages 1 to leafCount are leaves)
 * - numPairs: All leaves are full except last one (stored in MetadataPage)
 * - nextLeaf: Sequential layout (next leaf = pageId + 1)
 * 
 * OPTIMIZATION 2: Interleaved key-value storage
 * - Store as: k1,v1, k2,v2, k3,v3, ... instead of [all keys][all values]
 * - Makes leaf section a continuous stream across pages
 * - No logical boundary between pages in the data layout
 * - Entire leaf section can be viewed as one big array: [k1,v1,...,kN,vN]
 * 
 * This allows maximum space for data: 512 pairs instead of 510!
 */
struct LeafNode {
    int32_t pairs[MAX_LEAF_PAIRS * 2];  // Interleaved: [k1,v1, k2,v2, k3,v3, ...]
    
    LeafNode() {
        std::memset(pairs, 0, sizeof(pairs));
    }
    
    /**
     * Get key at index i
     */
    int32_t getKey(uint32_t index) const {
        return pairs[index * 2];  // Keys at even indices: 0, 2, 4, ...
    }
    
    /**
     * Get value at index i
     */
    int32_t getValue(uint32_t index) const {
        return pairs[index * 2 + 1];  // Values at odd indices: 1, 3, 5, ...
    }
    
    /**
     * Set key-value pair at index i
     */
    void setPair(uint32_t index, int32_t key, int32_t value) {
        pairs[index * 2] = key;
        pairs[index * 2 + 1] = value;
    }
    
    /**
     * Get the number of pairs in this leaf
     * @param pageId Current page ID
     * @param leafCount Total number of leaf pages (from metadata)
     * @param lastLeafPairs Number of pairs in the last leaf (from metadata)
     * @return Number of valid key-value pairs in this leaf
     */
    static uint32_t getNumPairs(uint32_t pageId, uint32_t leafCount, uint32_t lastLeafPairs) {
        if (pageId == leafCount) {
            // Last leaf - may be partially filled
            return lastLeafPairs;
        } else {
            // All other leaves are completely filled
            return MAX_LEAF_PAIRS;
        }
    }
    
    /**
     * Binary search for a key in the leaf
     * @param key The key to search for
     * @param numPairs Number of valid pairs in this leaf
     * @return index if found, -1 if not found
     */
    int search(int key, uint32_t numPairs) const {
        int left = 0;
        int right = numPairs - 1;
        
        while (left <= right) {
            int mid = left + (right - left) / 2;
            int32_t midKey = getKey(mid);
            
            if (midKey == key) {
                return mid;
            } else if (midKey < key) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return -1;  // Not found
    }
    
    /**
     * Serialize leaf node to a buffer (interleaved key-value pairs)
     * @param buffer Output buffer (must be PAGE_SIZE bytes)
     * 
     * Layout: [k1,v1, k2,v2, k3,v3, ..., k512,v512]
     * This creates a continuous stream across all leaf pages
     */
    void serialize(char* buffer) const {
        // Write interleaved pairs directly
        std::memcpy(buffer, pairs, sizeof(pairs));
    }
    
    /**
     * Serialize leaf node to a Page
     */
    void serialize(Page& page) const {
        page.clear();
        serialize(page.getData());
    }
    
    /**
     * Deserialize leaf node from a buffer (interleaved key-value pairs)
     * @param buffer Input buffer (must be PAGE_SIZE bytes)
     */
    void deserialize(const char* buffer) {
        // Read interleaved pairs directly
        std::memcpy(pairs, buffer, sizeof(pairs));
    }
    
    /**
     * Deserialize leaf node from a Page
     */
    void deserialize(const Page& page) {
        deserialize(page.getData());
    }
};

/**
 * Internal node - contains keys and child pointers for routing
 * Uses "Largest Key" approach: keys[i] = largest key in children[i]
 * 
 * OPTIMIZATION: No metadata stored in nodes!
 * - No type, numKeys, firstChild, childCount fields
 * - All metadata stored in MetadataPage for entire tree
 * - Sequential children calculated from position
 * - Node key counts determined from level metadata
 * - Uses full 4096 bytes for 1024 keys per node!
 */
struct InternalNode {
    int32_t keys[MAX_INTERNAL_KEYS];  // Largest key of each child (except rightmost)
    
    InternalNode() {
        std::memset(keys, 0, sizeof(keys));
    }
    
    /**
     * Find the child index for a given key using "Largest Key" approach
     * keys[i] = largest key in children[i]
     * Search rule: if key <= keys[i], go to children[i]
     * Otherwise, go to rightmost child (no upper bound)
     * 
     * @param key The key to search for
     * @param numKeys Number of valid keys in this node (must be passed from metadata/context)
     * @return Index of the child to follow
     */
    uint32_t findChildIndex(int32_t key, uint32_t numKeys) const {
        // Binary search for the first key >= key
        int left = 0;
        int right = numKeys - 1;
        
        while (left <= right) {
            int mid = left + (right - left) / 2;
            
            if (key <= keys[mid]) {
                // Check if this is the first key >= key
                if (mid == 0 || key > keys[mid - 1]) {
                    return mid;  // Found: key belongs in children[mid]
                }
                right = mid - 1;  // Search left half
            } else {
                left = mid + 1;  // Search right half
            }
        }
        
        // Key is larger than all stored keys → rightmost child
        return numKeys;
    }
    
    /**
     * Serialize internal node to a Page (just keys, no metadata)
     */
    void serialize(Page& page) const {
        page.clear();
        char* data = page.getData();
        
        // Write all keys directly (MAX_INTERNAL_KEYS * 4 bytes)
        std::memcpy(data, keys, sizeof(keys));
    }
    
    /**
     * Deserialize internal node from a Page (just keys, no metadata)
     */
    void deserialize(const Page& page) {
        const char* data = page.getData();
        
        // Read all keys directly (MAX_INTERNAL_KEYS * 4 bytes)
        std::memcpy(keys, data, sizeof(keys));
    }
};

#endif // BTREENODE_HPP
