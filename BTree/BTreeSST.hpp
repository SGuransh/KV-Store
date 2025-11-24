#pragma once

#ifndef BTREESST_HPP
#define BTREESST_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "BTreeNode.hpp"
#include "../BufferPool/Page.hpp"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

/**
 * Context structure for passing build state between helper functions
 * This avoids having instance variables and makes the class stateless
 * OPTIMIZED: No leafNodes array - write directly to disk
 */
struct BuildContext {
    std::string fileName;
    int fd;                      // File descriptor for direct writes
    size_t leafNodeCount;       // Number of leaf nodes
    size_t* internalLevelSizes;
    size_t internalLevelCount;
    size_t totalInternalNodes;
    int32_t minKey;          // Smallest key in the SST (for range filtering)
    int32_t maxKey;          // Largest key in the SST (for range filtering)
    uint32_t treeHeight;     // Height of the B-Tree (number of internal levels + 1 for leaves)
    uint32_t lastLeafPairs;  // Number of pairs in the last leaf

    uint32_t* nodesPerLevel;     // Number of nodes at each level
    uint32_t* lastNodeKeys;      // Number of keys/pairs in last node at each level
    
    // Bloom filter metadata
    uint32_t bloomBits;          // Total number of bits in bloom filter
    uint32_t bloomBytes;         // Total bytes allocated for bloom filter
    uint32_t bloomHashCount;     // Number of hash functions used
    
    BuildContext(const std::string& fname) 
        : fileName(fname), fd(-1), leafNodeCount(0), internalLevelSizes(nullptr), internalLevelCount(0), totalInternalNodes(0),
          lastLeafPairs(0), nodesPerLevel(nullptr), lastNodeKeys(nullptr),
          bloomBits(0), bloomBytes(0), bloomHashCount(0) {}

    ~BuildContext() {
        cleanup();
    }
    
    void cleanup() {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
        
        if (internalLevelSizes) {
            delete[] internalLevelSizes;
            internalLevelSizes = nullptr;
        }
        if (nodesPerLevel) {
            delete[] nodesPerLevel;
            nodesPerLevel = nullptr;
        }
        if (lastNodeKeys) {
            delete[] lastNodeKeys;
            lastNodeKeys = nullptr;
        }
        leafNodeCount = 0;
        internalLevelCount = 0;
    }
};

/**
 * BTreeSST - A stateless class for building and querying B-Tree SST files
 * 
 * This class provides functionality to:
 * 1. Build a static B-Tree SST from sorted key-value pairs (bottom-up construction)
 * 2. Query the B-Tree SST with point queries and range scans
 * 3. Convert the SST to a sorted array
 * 
 * The class is stateless - the same object can be used to work with multiple SST files.
 * All operations take the filename as a parameter.
 */
class BTreeSST {
public:
    BTreeSST() = default;
    ~BTreeSST() = default;
    
    /**
     * Build B-Tree from sorted key-value pairs and write to disk
     * @param sortedData Vector of sorted key-value pairs
     * @param fileName The SST file to create
     * @param bitsPerEntry Bits per entry for bloom filter (default: 10)
     * @param hashCount Number of hash functions for bloom filter (default: 3)
     * @return true if successful, false otherwise
     */
    bool buildBTree(const std::vector<std::pair<int, int>>& sortedData, 
                    const std::string& fileName,
                    uint32_t bitsPerEntry = 10,
                    uint32_t hashCount = 3);
    
    /**
     * Point query - find value for a given key
     * @param key The key to search for
     * @param value Output parameter for the value
     * @param fileName The SST file to query
     * @param useBTreeSearch If true, use B-Tree traversal; if false, use binary search
     * @return true if key found, false otherwise
     */
    bool get(int key, int& value, const std::string& fileName, bool useBTreeSearch = true);
    
    /**
     * Range scan - find all key-value pairs in range [key1, key2]
     * @param key1 Start of range (inclusive)
     * @param key2 End of range (inclusive)
     * @param fileName The SST file to scan
     * @param useBTreeSearch If true, use B-Tree traversal; if false, scan all leaves
     * @return Vector of key-value pairs in range
     */
    std::vector<std::pair<int, int>> scan(int key1, int key2, const std::string& fileName, bool useBTreeSearch = true);
    
    /**
     * Convert SST to sorted array of key-value pairs
     * @param fileName The SST file to read
     * @return Vector of all key-value pairs in sorted order
     */
    std::vector<std::pair<int, int>> toSortedArray(const std::string& fileName);
    
    /**
     * Calculate required array sizes based on input data size and allocate
     * @param ctx Build context to populate
     * @param dataSize Number of key-value pairs
     */
    void calculateAndAllocateArrays(BuildContext& ctx, size_t dataSize);
    
    /**
     * Phase 1: Write leaf pages directly to disk
     * OPTIMIZED: Converts data to interleaved format and writes directly to disk
     * @param ctx Build context with file descriptor open
     * @param sortedData The sorted key-value pairs
     */
    int32_t* buildLeafNodes(BuildContext& ctx, const std::vector<std::pair<int, int>>& sortedData);
    
    /**
     * Phase 2: Build internal levels bottom-up
     * OPTIMIZED: Calculates largest keys from sortedData instead of reading from disk
     * @param ctx Build context with leaf pages already written to disk
     * @param sortedData The sorted key-value pairs (for extracting largest keys)
     */
    void buildInternalLevels(BuildContext& ctx, const int32_t* max_per_node);
    
    /**
     * Phase 3: Write Context metadata to disk
     * @param ctx Build context with complete tree
     * @param dataSize Number of key-value pairs in the dataset
     * @return true if successful, false otherwise
     */
    bool writeMetadata(BuildContext& ctx, size_t dataSize);
    bool getBTreeSearch(int32_t key, int& value, const std::string& filename, const MetadataPage& metadata);

    // === Bloom Filter Operations (Public for testing) ===
    
    /**
     * Build bloom filter from sorted data as a simple bit array
     * @param sortedData Vector of sorted key-value pairs
     * @param bitsPerEntry Number of bits to allocate per entry
     * @param hashCount Number of hash functions to use
     * @return Vector of bytes representing the bloom filter bit array
     */
    std::vector<uint8_t> buildBloomFilter(const std::vector<std::pair<int, int>>& sortedData,
                                           uint32_t bitsPerEntry,
                                           uint32_t hashCount);
    
    /**
     * Hash function for bloom filter using double hashing
     * @param key The key to hash
     * @param hashIndex Which hash function (0 to k-1)
     * @param numBits Total number of bits in bloom filter
     * @return Bit position in the bloom filter
     */
    uint32_t bloomHash(int32_t key, uint32_t hashIndex, uint32_t numBits);
    
    /**
     * Check if a bit is set in the bloom filter
     * @param bloomFilter The bloom filter bit array
     * @param position Bit position to check
     * @return true if bit is set, false otherwise
     */
    bool bloomFilterTestBit(const std::vector<uint8_t>& bloomFilter, uint32_t position);
    
    /**
     * Set a bit in the bloom filter
     * @param bloomFilter The bloom filter bit array
     * @param position Bit position to set
     */
    void bloomFilterSetBit(std::vector<uint8_t>& bloomFilter, uint32_t position);
    
    /**
     * Read metadata from SST file
     * @param fileName The SST file to read
     * @param metadata Output parameter for metadata
     * @return true if successful, false otherwise
     */
    bool readMetadata(const std::string& fileName, MetadataPage& metadata) const;

private:
    /**
     * Write bloom filter to end of SST file
     * @param fd File descriptor (must be open for writing)
     * @param bloomFilter The bloom filter bit array
     * @return true if successful, false otherwise
     */
    bool writeBloomFilter(int fd, const std::vector<uint8_t>& bloomFilter);
    
    /**
     * Read bloom filter from end of SST file
     * @param fileName The SST file to read
     * @param metadata The metadata containing bloom filter info
     * @param bloomFilter Output parameter for bloom filter bit array
     * @return true if successful, false otherwise
     */
    bool readBloomFilter(const std::string& fileName, const MetadataPage& metadata, 
                        std::vector<uint8_t>& bloomFilter);
    
    // === File Operations ===
    
    /**
     * Read a page from disk using pread
     * @param fileName The SST file to read
     * @param pageId The page ID to read
     * @param page Output parameter for the page data
     * @return true if successful, false otherwise
     */
    bool readPage(const std::string& fileName, uint32_t pageId, Page& page) const;
    
    /**
     * Write a single page to disk at specified page ID
     * @param fd File descriptor (kept open during bulk write operations)
     * @param pageId The page ID to write to
     * @param page The page data to write
     * @return true if successful, false otherwise
     */
    bool writePage(int fd, uint32_t pageId, const Page& page) const;
    
    // === Query Operations ===
    
    /**
     * B-Tree search mode - traverse from root to leaf
     * @param key The key to search for
     * @param value Output parameter for the value
     * @param fileName The SST file to query
     * @param metadata The metadata for this SST
     * @return true if found, false otherwise
     */
    /**
     * Binary search mode - search directly on leaf pages
     * @param key The key to search for
     * @param value Output parameter for the value
     * @param fileName The SST file to query
     * @param metadata The metadata for this SST
     * @return true if found, false otherwise
     */
    bool getBinarySearch(int key, int& value, const std::string& fileName, const MetadataPage& metadata);
};

#endif // BTREESST_HPP