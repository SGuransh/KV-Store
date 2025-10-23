// #include "AVL.cpp"
#include "Memtable_ds.hpp"
#include "FileOperations.hpp"
#include <iostream>
#include "Database.hpp"
#include "MemtableFactory.hpp"


    // Memtable_ds* engine;
    // std::string databaseName;
    // std::string databaseDirectory;
    // bool isOpen;
    // int nextFileNumber;

    Database::Database(int memtableCapacity) {
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
            std::cout << "No incomplete.txt file found" << std::endl;
            return true;
        }

        std::cout << "Loading incomplete.txt file..." << std::endl;
        std::vector<std::pair<int, int>> data = FileOperations::read_sst_file(incompleteFile);
        
        if (data.empty()) {
            std::cout << "Incomplete file is empty, removing it" << std::endl;
            FileOperations::remove_file(incompleteFile);
            return true;
        }

        bool loadSuccess = engine->load_from_sst(data);
        if (loadSuccess) {
            std::cout << "Successfully loaded " << data.size() << " entries from incomplete.txt" << std::endl;
            FileOperations::remove_file(incompleteFile);
            return true;
        } else {
            std::cout << "Failed to load data from incomplete.txt" << std::endl;
            return false;
        }
    }


    bool Database::open_database(const std::string& dbName) {
        std::cout << "Opening database: " << dbName << std::endl;

        if (dbName.empty()) {
            std::cout << "Error: Database name cannot be empty" << std::endl;
            return false;
        }

        // Basic validation for invalid characters
        for (char c : dbName) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || 
                c == '"' || c == '<' || c == '>' || c == '|') {
                std::cout << "Error: Database name contains invalid characters: " << dbName << std::endl;
                return false;
            }
        }

        databaseName = dbName;
        databaseDirectory = databaseName;

        if (!FileOperations::create_directory(databaseDirectory)) {
            std::cout << "Failed to create or access database directory: " << databaseDirectory << std::endl;
            databaseName.clear();
            databaseDirectory.clear();
            return false;
        }

        int fileCount = FileOperations::count_sst_files(databaseDirectory);
        nextFileNumber = fileCount + 1;
        
        engine->set_next_file_number(nextFileNumber);
        engine->set_database_directory(databaseDirectory);

        if (!load_incomplete_file()) {
            std::cout << "Failed to load incomplete data" << std::endl;
            databaseName.clear();
            databaseDirectory.clear();
            return false;
        }

        isOpen = true;
        std::cout << "Successfully opened database: " << databaseName << " at directory: " << databaseDirectory << std::endl;
        std::cout << "Found " << fileCount << " existing SST files, next file number: " << nextFileNumber << std::endl;
        return true;
    }

    bool Database::close_database() {
        std::cout << "Closing database: " << databaseName << std::endl;

        if (!isOpen || databaseName.empty()) {
            std::cout << "No database is currently open" << std::endl;
            return true;
        }

        bool success = true;

        if (engine->get_size() > 0) {
            std::cout << "Memtable contains " << engine->get_size() << " entries, flushing before close" << std::endl;
            
            bool isComplete = (engine->get_size() == engine->get_max_elements());
            
            if (isComplete) {
                if (!engine->flush_to_sst(nextFileNumber, true)) {
                    std::cout << "Error: Failed to flush complete memtable data during database close" << std::endl;
                    success = false;
                } else {
                    std::cout << "Successfully flushed complete memtable data to SST file " << nextFileNumber << ".txt" << std::endl;
                    nextFileNumber++;
                }
            } else {
                if (!engine->flush_to_sst(-1, false)) {
                    std::cout << "Error: Failed to flush incomplete memtable data during database close" << std::endl;
                    success = false;
                } else {
                    std::cout << "Successfully flushed incomplete memtable data to incomplete.txt" << std::endl;
                }
            }
        } else {
            std::cout << "Memtable is empty, no data to flush" << std::endl;
        }

        databaseName.clear();
        databaseDirectory.clear();
        isOpen = false;
        nextFileNumber = 1;

        if (success) {
            std::cout << "Database closed successfully" << std::endl;
        } else {
            std::cout << "Database closed with errors (data may have been lost)" << std::endl;
        }

        return success;
    }

    // Core database operations
    bool Database::insert(int key, int value) {
        if (!isOpen) {
            std::cout << "Error: Database is not open" << std::endl;
            return false;
        }
        auto result = engine->insert(key, value);
        if (result != nullptr) {
            // Sync the next file number in case a flush occurred during insert
            nextFileNumber = engine->get_next_file_number();
            return true;
        }
        return false;
    }

    bool Database::search(int key, int& value) {
        if (!isOpen) {
            std::cout << "Error: Database is not open" << std::endl;
            return false;
        }
        return engine->search(key, value);
    }

    std::vector<std::pair<int, int>> Database::range_scan(int key1, int key2) {
        if (!isOpen) {
            std::cout << "Error: Database is not open" << std::endl;
            return std::vector<std::pair<int, int>>();
        }
        return engine->range_scan(key1, key2);
    }

    // Getters
    int Database::get_size() const { return engine->get_size(); }
    int Database::get_max_elements() const { return engine->get_max_elements(); }
    bool Database::is_open() const { return isOpen; }
    std::string Database::get_database_name() const { return databaseName; }
