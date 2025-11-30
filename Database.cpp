// #include "AVL.cpp"
#include "Memtable_ds.hpp"
#include "FileOperations.hpp"
#include "DBConfig.hpp"
#include <iostream>
#include <limits>
#include <map>
#include "Database.hpp"
#include "MemtableFactory.hpp"
#include "BTree/BTreeSST.hpp"
#include "LSM/LSMTree.hpp"


    // Memtable_ds* engine;
    // std::string databaseName;
    // std::string databaseDirectory;
    // bool isOpen;
    // int nextFileNumber;

    Database::Database(int memtableCapacity, uint32_t bitsPerEntry, uint32_t hashCount) 
        : bloomBitsPerEntry(bitsPerEntry), bloomHashCount(hashCount), useBTreeSearch(true) {
        // engine = new AVL(memtableCapacity);
        engine = create_memtable(MemtableType::AVL, memtableCapacity);
        isOpen = false;
        nextFileNumber = 1;
    }

    Database::~Database() {
        if (isOpen) {
            close_database();
        }
    }

    bool Database::load_incomplete_file() {
        std::string incompleteFile = databaseDirectory + "/incomplete.txt";
        
        if (!FileOperations::file_exists(incompleteFile)) {
            VERBOSE_PRINT("No incomplete.txt file found");
            return true;
        }

        VERBOSE_PRINT("Loading incomplete.txt file...");
        std::vector<std::pair<int, int>> data = FileOperations::read_sst_file(incompleteFile);
        
        if (data.empty()) {
            VERBOSE_PRINT("Incomplete file is empty, removing it");
            FileOperations::remove_file(incompleteFile);
            return true;
        }

        bool loadSuccess = engine->load_from_sst(data);
        if (loadSuccess) {
            VERBOSE_PRINT("Successfully loaded " << data.size() << " entries from incomplete.txt");
            FileOperations::remove_file(incompleteFile);
            return true;
        } else {
            VERBOSE_PRINT("Failed to load data from incomplete.txt");
            return false;
        }
    }


    bool Database::open_database(const std::string& dbName) {
        VERBOSE_PRINT("Opening database: " << dbName);

        if (dbName.empty()) {
            VERBOSE_PRINT("Error: Database name cannot be empty");  // Keep error
            return false;
        }

        // Basic validation for invalid characters
        for (char c : dbName) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || 
                c == '"' || c == '<' || c == '>' || c == '|') {
                VERBOSE_PRINT("Error: Database name contains invalid characters: " << dbName);  // Keep error
                return false;
            }
        }

        databaseName = dbName;
        databaseDirectory = databaseName;

        if (!FileOperations::create_directory(databaseDirectory)) {
            VERBOSE_PRINT("Error: Failed to create or access database directory: " << databaseDirectory);  // Keep error
            databaseName.clear();
            databaseDirectory.clear();
            return false;
        }

        int fileCount = FileOperations::count_sst_files(databaseDirectory);
        nextFileNumber = fileCount + 1;
        
        engine->set_next_file_number(nextFileNumber);
        engine->set_database_directory(databaseDirectory);

        // Initialize LSMTree
        lsmTree = std::make_unique<LSMTree>(databaseDirectory);
        if (!lsmTree) {
            VERBOSE_PRINT("Error: Failed to initialize LSMTree");  // Keep error
            databaseName.clear();
            databaseDirectory.clear();
            return false;
        }

        if (!load_incomplete_file()) {
            VERBOSE_PRINT("Error: Failed to load incomplete data");  // Keep error
            databaseName.clear();
            databaseDirectory.clear();
            lsmTree.reset();
            return false;
        }

        isOpen = true;
        VERBOSE_PRINT("Successfully opened database: " << databaseName << " at directory: " << databaseDirectory);
        VERBOSE_PRINT("Found " << fileCount << " existing SST files, next file number: " << nextFileNumber);
        return true;
    }

    bool Database::close_database() {
        VERBOSE_PRINT("Closing database: " << databaseName);

        if (!isOpen || databaseName.empty()) {
            VERBOSE_PRINT("No database is currently open");
            return true;
        }

        bool success = true;

        if (engine->get_size() > 0) {
            VERBOSE_PRINT("Memtable contains " << engine->get_size() << " entries, flushing before close");
            
            bool isComplete = (engine->get_size() == engine->get_max_elements());
            
            if (isComplete) {
                // Collect all pairs from memtable
                std::vector<std::pair<int, int>> pairs = engine->range_scan(
                    std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
                
                if (!pairs.empty()) {
                    // Get next SST number from LSMTree
                    int sstNumber = lsmTree->getNextSSTNumber();
                    
                    // Create SST filename (just the name, not full path)
                    std::string sstFileName = "sst_" + std::to_string(sstNumber) + ".txt";
                    std::string sstFullPath = databaseDirectory + "/" + sstFileName;
                    
                    // Build B-Tree SST from sorted memtable data with bloom filter
                    BTreeSST sstBuilder;
                    if (!sstBuilder.buildBTree(pairs, sstFullPath, bloomBitsPerEntry, bloomHashCount)) {
                        VERBOSE_PRINT("Error: Failed to build B-Tree SST file during database close");
                        success = false;
                    } else {
                        VERBOSE_PRINT("Successfully created SST file: " << sstFullPath);
                        
                        // Add SST to LSMTree at Level 0 (pass just the filename)
                        if (!lsmTree->addSST(sstFileName, 0)) {
                            VERBOSE_PRINT("Error: Failed to add SST to LSMTree during database close");
                            success = false;
                        } else {
                            VERBOSE_PRINT("Successfully flushed complete memtable data to SST and added to LSMTree");
                        }
                    }
                }
            } else {
                // For incomplete data, still use the old method to write to incomplete.txt
                if (!engine->flush_to_sst(-1, false)) {
                    VERBOSE_PRINT("Error: Failed to flush incomplete memtable data during database close");
                    success = false;
                } else {
                    VERBOSE_PRINT("Successfully flushed incomplete memtable data to incomplete.txt");
                }
            }
        } else {
            VERBOSE_PRINT("Memtable is empty, no data to flush");
        }

        // Clean up LSMTree
        lsmTree.reset();
        
        databaseName.clear();
        databaseDirectory.clear();
        isOpen = false;
        nextFileNumber = 1;

        if (success) {
            VERBOSE_PRINT("Database closed successfully");
        } else {
            VERBOSE_PRINT("Database closed with errors (data may have been lost)");
        }

        return success;
    }

    // Core database operations
    bool Database::insert(int key, int value) {
        if (!isOpen) {
            VERBOSE_PRINT("Error: Database is not open");
            return false;
        }
        
        // Check if memtable is at capacity and needs to be flushed
        if (engine->get_size() >= engine->get_max_elements()) {
            VERBOSE_PRINT("Memtable is at capacity (" << engine->get_size() << "/" 
                      << engine->get_max_elements() << "), flushing to SST");
            
            // Collect all pairs from memtable
            std::vector<std::pair<int, int>> pairs = engine->range_scan(
                std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
            
            if (pairs.empty()) {
                VERBOSE_PRINT("Warning: Memtable reported as full but no pairs collected");
            } else {
                // Get next SST number from LSMTree
                int sstNumber = lsmTree->getNextSSTNumber();
                
                // Create SST filename (just the name, not full path)
                std::string sstFileName = "sst_" + std::to_string(sstNumber) + ".txt";
                std::string sstFullPath = databaseDirectory + "/" + sstFileName;
                
                // Build B-Tree SST from sorted memtable data with bloom filter
                BTreeSST sstBuilder;
                if (!sstBuilder.buildBTree(pairs, sstFullPath, bloomBitsPerEntry, bloomHashCount)) {
                    VERBOSE_PRINT("Error: Failed to build B-Tree SST file");
                    return false;
                }
                
                VERBOSE_PRINT("Successfully created SST file: " << sstFullPath);
                
                // Add SST to LSMTree at Level 0 (pass just the filename)
                if (!lsmTree->addSST(sstFileName, 0)) {
                    VERBOSE_PRINT("Error: Failed to add SST to LSMTree");
                    return false;
                }
                
                VERBOSE_PRINT("Successfully added SST to LSMTree Level 0");
                
                // Clear the memtable manually since we're bypassing the internal flush
                // We need to create a new memtable instance
                int maxElements = engine->get_max_elements();
                engine = create_memtable(MemtableType::AVL, maxElements);
                engine->set_database_directory(databaseDirectory);
                
                VERBOSE_PRINT("Memtable cleared and ready for new insertions");
                
                // Check if Level 0 needs compaction (2 or more SSTs)
                if (lsmTree->needsCompaction(0)) {
                    VERBOSE_PRINT("Level 0 has multiple SSTs, triggering compaction...");
                    if (lsmTree->compactLevel(0)) {
                        VERBOSE_PRINT("Successfully compacted Level 0");
                    } else {
                        VERBOSE_PRINT("Warning: Level 0 compaction failed");
                    }
                }
            }
        }
        
        auto result = engine->insert(key, value);
        if (result != nullptr) {
            return true;
        }
        return false;
    }

    bool Database::search(int key, int& value) {
        if (!isOpen) {
            VERBOSE_PRINT("Error: Database is not open");
            return false;
        }
        
        // First check memtable (most recent data)
        if (engine->search(key, value)) {
            return true;
        }
        
        // If not found in memtable, query LSMTree with search mode preference
        return lsmTree->get(key, value, useBTreeSearch);
    }

    std::vector<std::pair<int, int>> Database::range_scan(int key1, int key2) {
        if (!isOpen) {
            VERBOSE_PRINT("Error: Database is not open");
            return std::vector<std::pair<int, int>>();
        }
        
        // Get results from memtable (most recent data)
        std::vector<std::pair<int, int>> memtableResults = engine->range_scan(key1, key2);
        
        // Get results from LSMTree
        std::vector<std::pair<int, int>> lsmResults = lsmTree->scan(key1, key2);
        
        // Merge results with memtable taking precedence for duplicate keys
        // Use a map to handle deduplication
        std::map<int, int> mergedMap;
        
        // First add LSM results (older data)
        for (const auto& pair : lsmResults) {
            mergedMap[pair.first] = pair.second;
        }
        
        // Then add memtable results (newer data, overwrites duplicates)
        for (const auto& pair : memtableResults) {
            mergedMap[pair.first] = pair.second;
        }
        
        // Convert map back to vector
        std::vector<std::pair<int, int>> result;
        result.reserve(mergedMap.size());
        for (const auto& pair : mergedMap) {
            result.push_back(pair);
        }
        
        return result;
    }

    // Getters
    int Database::get_size() const { return engine->get_size(); }
    int Database::get_max_elements() const { return engine->get_max_elements(); }
    bool Database::is_open() const { return isOpen; }
    std::string Database::get_database_name() const { return databaseName; }

    bool Database::compact_level(int level) {
        if (!isOpen || !lsmTree) {
            VERBOSE_PRINT("Error: Database is not open");
            return false;
        }
        
        if (!lsmTree->needsCompaction(level)) {
            VERBOSE_PRINT("Level " << level << " does not need compaction");
            return true;
        }
        
        VERBOSE_PRINT("Compacting Level " << level << "...");
        bool success = lsmTree->compactLevel(level);
        
        if (success) {
            VERBOSE_PRINT("Successfully compacted Level " << level);
        } else {
            VERBOSE_PRINT("Failed to compact Level " << level);
        }
        
        return success;
    }
