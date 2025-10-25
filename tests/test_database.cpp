#include "../FileOperations.hpp"
#include "../Database.hpp"
#include <iostream>
#include <cassert>
#include <fstream>

int db_tests_passed = 0;
int db_tests_failed = 0;

void db_test_passed(const std::string& test_name) {
    db_tests_passed++;
    std::cout << "[DB] " << test_name << " PASSED!" << std::endl;
}

void db_test_failed(const std::string& test_name) {
    db_tests_failed++;
    std::cout << "[DB] " << test_name << " FAILED!" << std::endl;
}

void cleanup_test_directory(const std::string& dirName) {
    std::string command = "rm -rf " + dirName;
    system(command.c_str());
}

void test_basic_operations() {
    std::cout << "\n--- Basic Database Operations ---" << std::endl;
    cleanup_test_directory("test_db_basic");
    
    Database db(5);
    
    if (db.open_database("test_db_basic")) {
        db_test_passed("Database Open");
    } else {
        db_test_failed("Database Open");
        return;
    }
    
    if (db.insert(10, 100) && db.insert(20, 200)) {
        db_test_passed("Database Insert");
    } else {
        db_test_failed("Database Insert");
    }
    
    int value;
    if (db.search(10, value) && value == 100) {
        db_test_passed("Database Search");
    } else {
        db_test_failed("Database Search");
    }
    
    if (db.close_database()) {
        db_test_passed("Database Close");
    } else {
        db_test_failed("Database Close");
    }
    
    cleanup_test_directory("test_db_basic");
}

void test_open_with_size_type() {
    std::cout << "\n--- Open Database With Size and Type ---" << std::endl;
    cleanup_test_directory("test_db_size_type");

    int custom_size = 7;
    std::string custom_type = "AVL";
    Database db(1); // Initial size doesn't matter, will be changed on open

    if (db.open_database_with_size_type("test_db_size_type", custom_size, custom_type)) {
        db_test_passed("Open With Size/Type: Database Open");
        if (db.get_max_elements() == custom_size) {
            db_test_passed("Open With Size/Type: Correct Memtable Size");
        } else {
            db_test_failed("Open With Size/Type: Correct Memtable Size");
        }
        // Check type saved in config
        std::ifstream configFile("test_db_size_type/config.txt");
        std::string configLine;
        bool found_type = false;
        while (std::getline(configFile, configLine)) {
            if (configLine.find("memtable_type=") == 0) {
                std::string type = configLine.substr(14);
                if (type == custom_type) {
                    db_test_passed("Open With Size/Type: Correct Memtable Type Saved");
                    found_type = true;
                } else {
                    db_test_failed("Open With Size/Type: Correct Memtable Type Saved");
                }
            }
        }
        if (!found_type) {
            db_test_failed("Open With Size/Type: memtable_type in config");
        }
        configFile.close();
    } else {
        db_test_failed("Open With Size/Type: Database Open");
    }

    db.close_database();
    cleanup_test_directory("test_db_size_type");
}



void test_sst_operations() {
    std::cout << "\n--- SST File Operations ---" << std::endl;
    cleanup_test_directory("test_db_sst");
    
    // Create database with small capacity to force SST creation
    Database db(2);
    
    if (!db.open_database("test_db_sst")) {
        db_test_failed("SST Database Open");
        return;
    }
    
    // Insert data that will fill memtable and create SST file
    if (db.insert(1, 10) && db.insert(2, 20)) {
        // This should trigger flush to SST and create new memtable
        if (db.insert(3, 30) && db.insert(4, 40)) {
            db_test_passed("SST Auto-Flush on Insert");
        } else {
            db_test_failed("SST Auto-Flush on Insert");
        }
    } else {
        db_test_failed("SST Initial Insert");
    }
    
    // Test search in SST files
    int value;
    if (db.search(1, value) && value == 10 && 
        db.search(2, value) && value == 20) {
        db_test_passed("SST Search");
    } else {
        db_test_failed("SST Search");
    }
    
    // Test search in memtable
    if (db.search(3, value) && value == 30) {
        db_test_passed("Mixed Search (SST + Memtable)");
    } else {
        db_test_failed("Mixed Search (SST + Memtable)");
    }
    
    db.close_database();
    cleanup_test_directory("test_db_sst");
}

void test_range_scan() {
    std::cout << "\n--- Range Scan Operations ---" << std::endl;
    cleanup_test_directory("test_db_range");
    
    Database db(3);
    
    if (!db.open_database("test_db_range")) {
        db_test_failed("Range Scan Database Open");
        return;
    }
    
    // Insert data across multiple SST files and memtable
    for (int i = 1; i <= 7; i++) {
        db.insert(i, i * 10);
    }
    
    // Test inclusive range scan
    auto results = db.range_scan(2, 5);
    if (results.size() == 4 && 
        results[0].first == 2 && results[0].second == 20 &&
        results[1].first == 3 && results[1].second == 30 &&
        results[2].first == 4 && results[2].second == 40 &&
        results[3].first == 5 && results[3].second == 50) {
        db_test_passed("Range Scan Inclusive Endpoints");
    } else {
        db_test_failed("Range Scan Inclusive Endpoints");
    }
    
    // Test single key range
    auto single_result = db.range_scan(3, 3);
    if (single_result.size() == 1 && 
        single_result[0].first == 3 && single_result[0].second == 30) {
        db_test_passed("Range Scan Single Key");
    } else {
        db_test_failed("Range Scan Single Key");
    }
    
    // Test full range
    auto full_results = db.range_scan(1, 7);
    if (full_results.size() == 7) {
        bool all_correct = true;
        for (int i = 0; i < 7; i++) {
            if (full_results[i].first != i + 1 || full_results[i].second != (i + 1) * 10) {
                all_correct = false;
                break;
            }
        }
        if (all_correct) {
            db_test_passed("Range Scan Full Range");
        } else {
            db_test_failed("Range Scan Full Range");
        }
    } else {
        db_test_failed("Range Scan Full Range");
    }
    
    db.close_database();
    cleanup_test_directory("test_db_range");
}

void test_persistence() {
    std::cout << "\n--- Data Persistence ---" << std::endl;
    cleanup_test_directory("test_db_persist");
    
    // First session: insert data and close
    {
        Database db(3);
        if (!db.open_database("test_db_persist")) {
            db_test_failed("Persistence Database Open");
            return;
        }
        
        // Insert data that will create SST files
        for (int i = 1; i <= 5; i++) {
            db.insert(i, i * 100);
        }
        
        db.close_database();
    }
    
    // Second session: reopen and verify data
    {
        Database db(3);
        if (!db.open_database("test_db_persist")) {
            db_test_failed("Persistence Database Reopen");
            return;
        }
        
        // Verify all data is still accessible
        bool all_found = true;
        int value;
        for (int i = 1; i <= 5; i++) {
            if (!db.search(i, value) || value != i * 100) {
                all_found = false;
                break;
            }
        }
        
        if (all_found) {
            db_test_passed("Data Persistence");
        } else {
            db_test_failed("Data Persistence");
        }
        
        // Test range scan on persisted data
        auto results = db.range_scan(2, 4);
        if (results.size() == 3 &&
            results[0].first == 2 && results[0].second == 200 &&
            results[1].first == 3 && results[1].second == 300 &&
            results[2].first == 4 && results[2].second == 400) {
            db_test_passed("Persistence Range Scan");
        } else {
            db_test_failed("Persistence Range Scan");
        }
        
        db.close_database();
    }
    
    cleanup_test_directory("test_db_persist");
}

void test_file_number_sync() {
    std::cout << "\n--- File Number Synchronization ---" << std::endl;
    cleanup_test_directory("test_db_filenum");
    
    Database db(2);
    
    if (!db.open_database("test_db_filenum")) {
        db_test_failed("File Sync Database Open");
        return;
    }
    
    // Insert data to create multiple SST files
    for (int i = 1; i <= 6; i++) {
        db.insert(i, i * 10);
    }
    
    // Close database (should create another SST file without overwriting)
    db.close_database();
    
    // Reopen and verify all data is intact
    if (!db.open_database("test_db_filenum")) {
        db_test_failed("File Sync Database Reopen");
        return;
    }
    
    // Verify all keys are still accessible
    bool all_found = true;
    int value;
    for (int i = 1; i <= 6; i++) {
        if (!db.search(i, value) || value != i * 10) {
            all_found = false;
            break;
        }
    }
    
    if (all_found) {
        db_test_passed("File Number Synchronization");
    } else {
        db_test_failed("File Number Synchronization");
    }
    
    db.close_database();
    cleanup_test_directory("test_db_filenum");
}
void test_open_existing_and_config_size() {
    std::cout << "\n--- Test Open Existing DB and Config Size ---" << std::endl;
    std::string dbName = "test_db_config";
    cleanup_test_directory(dbName);

    // Step 1: Create new database with custom size and close it
    int initial_size = 22;
    {
        Database db(initial_size);
        if (db.open_database_with_size_type(dbName, initial_size, "AVL")) {
            db_test_passed("Create DB with custom size");
            if (db.get_max_elements() == initial_size) {
                db_test_passed("Initial DB memtable size correct");
            } else {
                db_test_failed("Initial DB memtable size correct");
            }
        } else {
            db_test_failed("Create DB with custom size");
        }
        db.close_database();
    }

    // Step 2: Open existing database with a different size, should load from config
    int requested_size = 100; // Should be ignored, config should be used
    {
        Database db(requested_size);
        if (db.open_database_with_size_type(dbName, requested_size, "AVL")) {
            db_test_passed("Open existing DB with different requested size");
            int loaded_size = db.get_max_elements();
            if (loaded_size == requested_size) {
                db_test_passed("Loaded and updated memtable size from config.txt");
            } else {
                db_test_failed("Loaded memtable size from config.txt");
                std::cout << "Expected: " << initial_size << ", Got: " << loaded_size << std::endl;
            }
        } else {
            db_test_failed("Open existing DB with different requested size");
        }
        db.close_database();
    }

    // Step 3: Check config.txt value
    std::ifstream configFile(dbName + "/config.txt");
    std::string configLine;
    bool found = false;
    while (std::getline(configFile, configLine)) {
        if (configLine.find("memtable_size=") == 0) {
            int config_size = std::stoi(configLine.substr(14));
            if (config_size == requested_size) {
                db_test_passed("Config file saved correct memtable size");
                found = true;
            } else {
                db_test_failed("Config file saved correct memtable size");
            }
        }
    }
    if (!found) {
        db_test_failed("Config file contains memtable_size");
    }
    configFile.close();

    cleanup_test_directory(dbName);
}

int run_database_tests() {
    std::cout << "\n========== DATABASE TESTS ==========" << std::endl;
    
    test_basic_operations();
    test_sst_operations();
    test_range_scan();
    test_persistence();
    test_file_number_sync();
    test_open_with_size_type();
    test_open_existing_and_config_size();
    
    std::cout << "\n=== DATABASE TEST SUMMARY ===" << std::endl;
    std::cout << "Database Tests Passed: " << db_tests_passed << std::endl;
    std::cout << "Database Tests Failed: " << db_tests_failed << std::endl;
    
    return db_tests_failed;
}


int main() {
    return run_database_tests();
}