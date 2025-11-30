#pragma once

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "FileOperations.hpp"
#include "AVL.hpp" 
#include <string>
#include <vector>
#include "Memtable_ds.hpp"
#include "LSM/LSMTree.hpp"
#include "BufferPool/BufferPool.hpp"
#include <memory>

class Database {
private:
    std::unique_ptr<Memtable_ds> engine;
    std::unique_ptr<LSMTree> lsmTree;
    std::unique_ptr<BufferPool> bufferPool;  // Buffer pool for page caching
    std::string databaseName;
    std::string databaseDirectory;
    bool isOpen;
    int nextFileNumber;
    
    // Bloom filter configuration
    uint32_t bloomBitsPerEntry;   // Default: 10 bits per entry
    uint32_t bloomHashCount;       // Default: 3 hash functions
    
    // Search mode configuration
    bool useBTreeSearch;           // Default: true (use B-Tree search), false (use binary search)

    bool load_incomplete_file();

public:
    Database(int memtableCapacity = 1000, uint32_t bitsPerEntry = 10, uint32_t hashCount = 3);
    ~Database();

    bool open_database(const std::string& dbName);
    bool close_database();

    bool insert(int key, int value);
    bool search(int key, int& value);
    std::vector<std::pair<int, int>> range_scan(int key1, int key2);
    
    // LSM operations
    bool compact_level(int level);

    // Bloom filter getters
    uint32_t getBloomBitsPerEntry() const { return bloomBitsPerEntry; }
    uint32_t getBloomHashCount() const { return bloomHashCount; }
    
    // Search mode configuration
    void setUseBTreeSearch(bool useBTree) { useBTreeSearch = useBTree; }
    bool getUseBTreeSearch() const { return useBTreeSearch; }

    // Getters
    int get_size() const;
    int get_max_elements() const;
    bool is_open() const;
    std::string get_database_name() const;
};

#endif // DATABASE_HPP