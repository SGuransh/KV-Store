#pragma once

#ifndef LSMTREE_HPP
#define LSMTREE_HPP

#include <string>
#include <vector>
#include <memory>
#include "SSTMetadata.hpp"
#include "../BTree/BTreeSST.hpp"
#include "../BufferPool/BufferPool.hpp"

/**
 * LSMTree - Log-Structured Merge Tree with fixed size ratio of 2
 * 
 * Manages multiple levels of SST files where each level can hold SSTs
 * approximately twice the size of the previous level. Automatically
 * triggers compaction when two SSTs exist at the same level.
 */
class LSMTree {
private:
    std::string dbDirectory;                          // Database directory path
    std::vector<std::vector<SSTMetadata>> levels;     // levels[i] = SSTs at level i
    BTreeSST sstBuilder;                              // For building and querying SSTs
    int nextSSTNumber;                                // Next unique SST number to assign

    // Manifest file management
    bool loadManifest();
    bool saveManifest();

    // Merge operation (used by compaction)
    bool mergeTwoSSTs(const std::string& sst1, const std::string& sst2,
                      const std::string& outputSST, int targetLevel);

public:
    // Compaction operations (public for Database class)
    bool needsCompaction(int level) const;
    bool compactLevel(int level);
    /**
     * Constructor - Initialize LSMTree for a database directory
     * @param directory Path to the database directory
     * @param pool Optional BufferPool pointer for caching (default: nullptr)
     */
    LSMTree(const std::string& directory, BufferPool* pool = nullptr);
    
    /**
     * Destructor - Cleanup resources
     */
    ~LSMTree();

    /**
     * Add a new SST file to the LSM tree
     * @param sstFile Path to the SST file
     * @param level Level to add the SST to (default: 0)
     * @return true if successful, false otherwise
     */
    bool addSST(const std::string& sstFile, int level = 0);

    /**
     * Point query - find value for a given key
     * @param key The key to search for
     * @param value Output parameter for the value
     * @param useBTreeSearch If true, use B-Tree search; if false, use binary search
     * @return true if key found, false otherwise
     */
    bool get(int key, int& value, bool useBTreeSearch = true);

    /**
     * Range scan - find all key-value pairs in range [key1, key2]
     * @param key1 Start of range (inclusive)
     * @param key2 End of range (inclusive)
     * @return Vector of deduplicated key-value pairs in range
     */
    std::vector<std::pair<int, int>> scan(int key1, int key2);

    /**
     * Get the next SST number to use
     * @return Next SST number
     */
    int getNextSSTNumber();
    
    /**
     * Test method - expose mergeTwoSSTs for testing
     * @param sst1 First SST file name
     * @param sst2 Second SST file name
     * @param outputSST Output SST file name
     * @param targetLevel Target level for output SST
     * @return true if successful, false otherwise
     */
    bool testMergeTwoSSTs(const std::string& sst1, const std::string& sst2,
                          const std::string& outputSST, int targetLevel) {
        return mergeTwoSSTs(sst1, sst2, outputSST, targetLevel);
    }
};

#endif // LSMTREE_HPP
