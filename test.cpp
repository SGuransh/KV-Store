// #include "AVL.cpp"
#include "Database.cpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>

// Global test counters
int total_tests = 0;
int passed_tests = 0;
int failed_tests = 0;

// Test result tracking functions
void test_passed(const std::string& test_name) {
    total_tests++;
    passed_tests++;
    std::cout << test_name << " PASSED!" << std::endl;
}

void test_failed(const std::string& test_name) {
    total_tests++;
    failed_tests++;
    std::cout << test_name << " FAILED!" << std::endl;
}

void print_test_summary() {
    std::cout << "\n=== TEST SUMMARY ===" << std::endl;
    std::cout << "Total Tests: " << total_tests << std::endl;
    std::cout << "Passed: " << passed_tests << std::endl;
    std::cout << "Failed: " << failed_tests << std::endl;
    std::cout << "Success Rate: " << (total_tests > 0 ? (passed_tests * 100.0 / total_tests) : 0) << "%" << std::endl;
}

// Helper functions for testing
void cleanup_test_directory(const std::string& dirName) {
    std::string command = "rm -rf " + dirName;
    system(command.c_str());
}

bool directory_exists(const std::string& dirName) {
    struct stat st;
    return (stat(dirName.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
}

bool file_exists(const std::string& fileName) {
    struct stat st;
    return (stat(fileName.c_str(), &st) == 0) && S_ISREG(st.st_mode);
}

int count_sst_files_in_directory(const std::string& dirName) {
    if (!directory_exists(dirName)) {
        return 0;
    }
    
    int count = 0;
    DIR* dir = opendir(dirName.c_str());
    if (dir == nullptr) {
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".txt") {
            std::string numberPart = filename.substr(0, filename.length() - 4);
            bool isNumber = true;
            for (char c : numberPart) {
                if (!std::isdigit(c)) {
                    isNumber = false;
                    break;
                }
            }
            if (isNumber && !numberPart.empty()) {
                count++;
            }
        }
    }
    closedir(dir);
    return count;
}

std::vector<std::pair<int, int>> read_sst_file(const std::string& filePath) {
    std::vector<std::pair<int, int>> content;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return content;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        int key, value;
        if (iss >> key >> value) {
            content.push_back({key, value});
        }
    }
    file.close();
    return content;
}

bool verify_sst_file_sorted(const std::vector<std::pair<int, int>>& content) {
    for (size_t i = 1; i < content.size(); i++) {
        if (content[i-1].first >= content[i].first) {
            return false;
        }
    }
    return true;
}

// Basic functionality tests
void run_basic_tests() {
    std::cout << "\n=== Basic Functionality Tests ===" << std::endl;
    
    int maxElements = 6;
    Memtable_ds* memtable = new AVL(maxElements);
    memtable->insert(16, 100);
    memtable->insert(7, 200);
    memtable->insert(53, 300);
    memtable->insert(30, 400);
    memtable->insert(55, 500);
    memtable->insert(29, 600); 

    // Test 1: Size check
    if (memtable->get_size() != 6) {
        test_failed("Basic Test 1: Size check");
    } else {
        test_passed("Basic Test 1: Size check");
    } 

    // Test 2: Root existence and key check
    if (memtable->get_root() == nullptr) {
        test_failed("Basic Test 2: Root existence check");
    } else if (memtable->get_root()->key != 30) {
        std::cout << "Root key check failed! Expected 30, got " << memtable->get_root()->key << std::endl;
        test_failed("Basic Test 2: Root key check");
    } else {
        test_passed("Basic Test 2: Root key check");
    }

    // Test 3: Max elements check
    if (memtable->get_max_elements() != 6) {
        test_failed("Basic Test 3: Max elements check");
    } else {
        test_passed("Basic Test 3: Max elements check");
    }

    printf("Inorder traversal of the AVL tree: ");
    std::vector<int> inorder_result = memtable->inorder();
    for (int key : inorder_result) {
        std::cout << key << " ";
    }
    std::cout << std::endl;

    delete memtable;
}

// Range scan tests
void run_range_scan_tests() {
    std::cout << "\n=== Range Scan Tests ===" << std::endl;
    
    int maxElements = 6;
    Memtable_ds* memtable = new AVL(maxElements);
    memtable->insert(16, 100);
    memtable->insert(7, 200);
    memtable->insert(53, 300);
    memtable->insert(30, 400);
    memtable->insert(55, 500);
    memtable->insert(29, 600);
    
    // Test 1: Normal range scan with multiple elements
    std::cout << "\nTest 1: Range scan from 10 to 50" << std::endl;
    std::vector<std::pair<int, int>> result1 = memtable->range_scan(10, 50);
    std::cout << "Expected elements with keys: 16, 29, 30" << std::endl;
    std::cout << "Found " << result1.size() << " elements: ";
    for (const auto& pair : result1) {
        std::cout << "(" << pair.first << "," << pair.second << ") ";
    }
    std::cout << std::endl;
    if (result1.size() == 3 && result1[0].first == 16 && result1[1].first == 29 && result1[2].first == 30) {
        test_passed("Range Scan Test 1: Normal range scan");
    } else {
        test_failed("Range Scan Test 1: Normal range scan");
    }

    // Test 2: Range scan with single element
    std::cout << "\nTest 2: Range scan from 28 to 31" << std::endl;
    std::vector<std::pair<int, int>> result2 = memtable->range_scan(28, 31);
    std::cout << "Expected elements with keys: 29, 30" << std::endl;
    std::cout << "Found " << result2.size() << " elements: ";
    for (const auto& pair : result2) {
        std::cout << "(" << pair.first << "," << pair.second << ") ";
    }
    std::cout << std::endl;
    if (result2.size() == 2 && result2[0].first == 29 && result2[1].first == 30) {
        test_passed("Range Scan Test 2: Range with two elements");
    } else {
        test_failed("Range Scan Test 2: Range with two elements");
    }

    // Test 3: Empty range (no elements in range)
    std::cout << "\nTest 3: Range scan from 1 to 5 (empty range)" << std::endl;
    std::vector<std::pair<int, int>> result3 = memtable->range_scan(1, 5);
    std::cout << "Expected: 0 elements" << std::endl;
    std::cout << "Found " << result3.size() << " elements" << std::endl;
    if (result3.size() == 0) {
        test_passed("Range Scan Test 3: Empty range");
    } else {
        test_failed("Range Scan Test 3: Empty range");
    }

    // Test 4: Range scan with all elements
    std::cout << "\nTest 4: Range scan from 0 to 100 (all elements)" << std::endl;
    std::vector<std::pair<int, int>> result4 = memtable->range_scan(0, 100);
    std::cout << "Expected: 6 elements" << std::endl;
    std::cout << "Found " << result4.size() << " elements: ";
    for (const auto& pair : result4) {
        std::cout << "(" << pair.first << "," << pair.second << ") ";
    }
    std::cout << std::endl;
    if (result4.size() == 6) {
        test_passed("Range Scan Test 4: All elements range");
    } else {
        test_failed("Range Scan Test 4: All elements range");
    }

    // Test 5: Invalid range (key1 >= key2)
    std::cout << "\nTest 5: Invalid range scan from 50 to 10" << std::endl;
    std::vector<std::pair<int, int>> result5 = memtable->range_scan(50, 10);
    std::cout << "Expected: 0 elements (invalid range)" << std::endl;
    std::cout << "Found " << result5.size() << " elements" << std::endl;
    if (result5.size() == 0) {
        test_passed("Range Scan Test 5: Invalid range");
    } else {
        test_failed("Range Scan Test 5: Invalid range");
    }

    // Test 6: Equal boundaries (key1 == key2)
    std::cout << "\nTest 6: Equal boundaries range scan from 30 to 30" << std::endl;
    std::vector<std::pair<int, int>> result6 = memtable->range_scan(30, 30);
    std::cout << "Expected: 0 elements (equal boundaries)" << std::endl;
    std::cout << "Found " << result6.size() << " elements" << std::endl;
    if (result6.size() == 0) {
        test_passed("Range Scan Test 6: Equal boundaries");
    } else {
        test_failed("Range Scan Test 6: Equal boundaries");
    }

    // Test 7: Boundary exclusion test
    std::cout << "\nTest 7: Boundary exclusion test from 16 to 30" << std::endl;
    std::vector<std::pair<int, int>> result7 = memtable->range_scan(16, 30);
    std::cout << "Expected elements with keys: 29 (16 and 30 should be excluded)" << std::endl;
    std::cout << "Found " << result7.size() << " elements: ";
    for (const auto& pair : result7) {
        std::cout << "(" << pair.first << "," << pair.second << ") ";
    }
    std::cout << std::endl;
    if (result7.size() == 1 && result7[0].first == 29) {
        test_passed("Range Scan Test 7: Boundary exclusion");
    } else {
        test_failed("Range Scan Test 7: Boundary exclusion");
    }

    // Test 8: Verify sorted order
    std::cout << "\nTest 8: Verify results are in sorted order" << std::endl;
    std::vector<std::pair<int, int>> result8 = memtable->range_scan(0, 100);
    bool sorted = true;
    for (size_t i = 1; i < result8.size(); i++) {
        if (result8[i-1].first >= result8[i].first) {
            sorted = false;
            break;
        }
    }
    if (sorted) {
        test_passed("Range Scan Test 8: Sorted order verification");
    } else {
        test_failed("Range Scan Test 8: Sorted order verification");
    }

    delete memtable;
}

// Clear memtable tests
void run_clear_memtable_tests() {
    std::cout << "\n=== Clear Memtable Tests ===" << std::endl;
    
    // Create AVL instance
    AVL avl(5);
    
    // Insert some data
    std::cout << "\nInserting test data..." << std::endl;
    avl.insert(10, 100);
    avl.insert(5, 50);
    avl.insert(15, 150);
    
    // Test 1: Verify data is present before clear
    std::cout << "Size before clear: " << avl.get_size() << std::endl;
    std::cout << "Root before clear: " << (avl.get_root() ? "exists" : "null") << std::endl;
    
    int value;
    bool found = avl.search(10, value);
    std::cout << "Search for key 10: " << (found ? "found" : "not found") << std::endl;
    
    if (avl.get_size() == 3 && avl.get_root() != nullptr && found) {
        test_passed("Clear Memtable Test 1: Data present before clear");
    } else {
        test_failed("Clear Memtable Test 1: Data present before clear");
    }
    
    // Open database to enable clear_memtable functionality
    std::cout << "\nOpening database..." << std::endl;
    bool opened = avl.open_database("test_db");
    std::cout << "Database opened: " << (opened ? "success" : "failed") << std::endl;
    
    // Insert data again after opening database
    avl.insert(20, 200);
    avl.insert(25, 250);
    
    std::cout << "\nAfter adding more data:" << std::endl;
    std::cout << "Size: " << avl.get_size() << std::endl;
    std::cout << "Root: " << (avl.get_root() ? "exists" : "null") << std::endl;
    
    // Test flush_to_sst which internally calls clear_memtable
    std::cout << "\nTesting flush_to_sst (which calls clear_memtable)..." << std::endl;
    bool flushed = avl.flush_to_sst();
    std::cout << "Flush result: " << (flushed ? "success" : "failed") << std::endl;
    
    // Test 2: Verify memtable is cleared
    std::cout << "\nAfter flush (clear_memtable should have been called):" << std::endl;
    std::cout << "Size after clear: " << avl.get_size() << std::endl;
    std::cout << "Root after clear: " << (avl.get_root() ? "exists" : "null") << std::endl;
    
    // Verify data is no longer searchable in memtable
    found = avl.search(20, value);
    std::cout << "Search for key 20 after clear: " << (found ? "found" : "not found") << std::endl;
    
    if (avl.get_size() == 0 && avl.get_root() == nullptr && !found) {
        test_passed("Clear Memtable Test 2: Memtable cleared after flush");
    } else {
        test_failed("Clear Memtable Test 2: Memtable cleared after flush");
    }
    
    // Test 3: Test that we can insert new data after clear
    std::cout << "\nTesting insertion after clear..." << std::endl;
    auto result = avl.insert(30, 300);
    std::cout << "Insert after clear: " << (result ? "success" : "failed") << std::endl;
    std::cout << "Size after new insert: " << avl.get_size() << std::endl;
    
    if (result && avl.get_size() == 1) {
        test_passed("Clear Memtable Test 3: Insert after clear");
    } else {
        test_failed("Clear Memtable Test 3: Insert after clear");
    }
    
    // Close database
    avl.close_database();
    cleanup_test_directory("test_db");
}

// Database lifecycle tests
void run_database_lifecycle_tests() {
    std::cout << "\n=== Database Lifecycle Tests ===" << std::endl;
    
    // Test 1: Open new database
    std::cout << "\nTest 1: Open new database" << std::endl;
    std::string testDbName = "test_lifecycle_db";
    cleanup_test_directory(testDbName);
    
    // AVL avl(5);
    // bool openResult = avl.open_database(testDbName);
    Database db(5);
    bool openResult = db.openDatabase(testDbName);

    std::cout << "Open result: " << (openResult ? "success" : "failed") << std::endl;
    std::cout << "Directory created: " << (directory_exists(testDbName) ? "yes" : "no") << std::endl;
    
    if (openResult && directory_exists(testDbName)) {
        test_passed("Database Lifecycle Test 1: Open new database");
    } else {
        test_failed("Database Lifecycle Test 1: Open new database");
    }
    
    // Test 2: Insert data and close
    std::cout << "\nTest 2: Insert data and close database" << std::endl;
    db.put(10, 100);
    db.put(20, 200);
    db.put(30, 300);
    
    bool closeResult = db.closeDatabase();
    std::cout << "Close result: " << (closeResult ? "success" : "failed") << std::endl;
    
    int sstCount = count_sst_files_in_directory(testDbName);
    std::cout << "SST files created: " << sstCount << std::endl;
    
    if (closeResult && sstCount == 1) {
        test_passed("Database Lifecycle Test 2: Close database with SST creation");
    } else {
        test_failed("Database Lifecycle Test 2: Close database with SST creation");
    }
    
    cleanup_test_directory(testDbName);
}

// Auto flush tests
void run_auto_flush_tests() {
    std::cout << "\n=== Auto Flush Tests ===" << std::endl;
    
    std::string testDbName = "test_auto_flush";
    cleanup_test_directory(testDbName);
    
    int maxCapacity = 3;
    AVL avl(maxCapacity);
    
    // Test 1: Open database
    bool openResult = avl.open_database(testDbName);
    std::cout << "Database opened: " << (openResult ? "success" : "failed") << std::endl;
    
    if (openResult) {
        test_passed("Auto Flush Test 1: Database opened");
    } else {
        test_failed("Auto Flush Test 1: Database opened");
    }
    
    // Insert data up to capacity - 1 (should not trigger flush)
    avl.insert(10, 100);
    avl.insert(20, 200);
    
    // Test 2: Verify memtable has data and no SST files created yet
    std::cout << "Size before reaching capacity: " << avl.get_size() << std::endl;
    int sstCount1 = count_sst_files_in_directory(testDbName);
    std::cout << "SST files before capacity: " << sstCount1 << std::endl;
    
    if (avl.get_size() == 2 && sstCount1 == 0) {
        test_passed("Auto Flush Test 2: No flush before capacity");
    } else {
        test_failed("Auto Flush Test 2: No flush before capacity");
    }
    
    // Insert one more element to reach capacity (should NOT trigger flush yet)
    avl.insert(30, 300);
    
    // Verify no flush occurred yet (memtable is at capacity but not over)
    int sstCount2 = count_sst_files_in_directory(testDbName);
    std::cout << "SST files after reaching capacity: " << sstCount2 << std::endl;
    std::cout << "Size after reaching capacity: " << avl.get_size() << std::endl;
    
    // Now insert one more element to trigger auto-flush
    avl.insert(40, 400);
    
    // Verify automatic flush occurred
    int sstCount3 = count_sst_files_in_directory(testDbName);
    std::cout << "SST files after exceeding capacity: " << sstCount3 << std::endl;
    std::cout << "Size after auto-flush: " << avl.get_size() << std::endl;
    
    // Test 3: Verify SST file contains the flushed data
    std::string sstFile = testDbName + "/1.txt";
    if (file_exists(sstFile)) {
        std::vector<std::pair<int, int>> sstContent = read_sst_file(sstFile);
        std::cout << "SST file contains " << sstContent.size() << " entries" << std::endl;
        std::cout << "SST file is sorted: " << (verify_sst_file_sorted(sstContent) ? "yes" : "no") << std::endl;
        
        if (sstContent.size() == 3 && verify_sst_file_sorted(sstContent) && avl.get_size() == 1) {
            test_passed("Auto Flush Test 3: SST file content verification");
        } else {
            test_failed("Auto Flush Test 3: SST file content verification");
        }
    } else {
        test_failed("Auto Flush Test 3: SST file content verification (file not found)");
    }
    
    // Clean up
    avl.close_database();
    cleanup_test_directory(testDbName);
}

// Integration workflow tests
void run_integration_tests() {
    std::cout << "\n=== Integration Workflow Tests ===" << std::endl;
    
    std::string testDbName = "test_integration";
    cleanup_test_directory(testDbName);
    
    int maxCapacity = 4;
    AVL avl(maxCapacity);
    
    // Complete workflow test
    std::cout << "\nRunning complete workflow test..." << std::endl;
    
    // Step 1: Open database
    bool openResult = avl.open_database(testDbName);
    std::cout << "1. Database opened: " << (openResult ? "success" : "failed") << std::endl;
    
    if (openResult) {
        test_passed("Integration Test 1: Database opened");
    } else {
        test_failed("Integration Test 1: Database opened");
    }
    
    // Step 2: Insert data up to capacity (should not trigger flush yet)
    std::vector<std::pair<int, int>> insertedData = {
        {50, 500}, {30, 300}, {70, 700}, {20, 200}
    };
    
    for (const auto& pair : insertedData) {
        auto result = avl.insert(pair.first, pair.second);
        if (!result) {
            std::cout << "Insert failed for key " << pair.first << std::endl;
        }
    }
    
    // Check that memtable is at capacity but no flush yet
    int sstCountBeforeFlush = count_sst_files_in_directory(testDbName);
    std::cout << "2a. SST files after filling to capacity: " << sstCountBeforeFlush << std::endl;
    std::cout << "    Memtable size: " << avl.get_size() << std::endl;
    
    // Now insert one more element to trigger auto-flush
    auto flushTriggerResult = avl.insert(10, 100);
    
    // Verify flush occurred
    int sstCount = count_sst_files_in_directory(testDbName);
    std::cout << "2b. SST files after triggering flush: " << sstCount << std::endl;
    std::cout << "    Memtable size after flush: " << avl.get_size() << std::endl;
    
    if (sstCount >= 1 && avl.get_size() == 1) {
        test_passed("Integration Test 2: Auto flush occurred");
    } else {
        test_failed("Integration Test 2: Auto flush occurred");
    }
    
    // Step 3: Verify SST file contains correct data in sorted order
    std::string sstFile = testDbName + "/1.txt";
    if (file_exists(sstFile)) {
        std::vector<std::pair<int, int>> sstContent = read_sst_file(sstFile);
        std::cout << "3. SST file entries: " << sstContent.size() << std::endl;
        std::cout << "   SST file is sorted: " << (verify_sst_file_sorted(sstContent) ? "yes" : "no") << std::endl;
        
        // Print SST content
        std::cout << "   SST content: ";
        for (const auto& pair : sstContent) {
            std::cout << "(" << pair.first << "," << pair.second << ") ";
        }
        std::cout << std::endl;
        
        if (sstContent.size() == 4 && verify_sst_file_sorted(sstContent)) {
            test_passed("Integration Test 3: SST file verification");
        } else {
            test_failed("Integration Test 3: SST file verification");
        }
    } else {
        test_failed("Integration Test 3: SST file verification (file not found)");
    }
    
    // Step 4: Test range scan
    avl.insert(40, 400);
    avl.insert(60, 600);
    
    std::vector<std::pair<int, int>> rangeResult = avl.range_scan(35, 65);
    std::cout << "4. Range scan (35, 65) found " << rangeResult.size() << " elements in memtable" << std::endl;
    
    if (rangeResult.size() == 2) {
        test_passed("Integration Test 4: Range scan functionality");
    } else {
        test_failed("Integration Test 4: Range scan functionality");
    }
    
    // Step 5: Close database
    bool closeResult = avl.close_database();
    std::cout << "5. Database closed: " << (closeResult ? "success" : "failed") << std::endl;
    
    // Verify final state
    int finalSstCount = count_sst_files_in_directory(testDbName);
    std::cout << "   Final SST files: " << finalSstCount << std::endl;
    
    if (closeResult && finalSstCount >= 2) {
        test_passed("Integration Test 5: Database close with final flush");
    } else {
        test_failed("Integration Test 5: Database close with final flush");
    }
    
    // Clean up
    cleanup_test_directory(testDbName);
}


void run_sst_search_tests() {
    std::cout << "\n=== SST Search Tests ===" << std::endl;

    std::string testDbName = "test_sst_search";
    cleanup_test_directory(testDbName);
    mkdir(testDbName.c_str(), 0777);

    // --- Create first fake SST file (1.txt)
    std::string sstFilePath1 = testDbName + "/1.txt";
    std::ofstream sstFile1(sstFilePath1);
    if (sstFile1.is_open()) {
        sstFile1 << "10 100\n";
        sstFile1 << "20 200\n";
        sstFile1 << "30 300\n";
        sstFile1.close();
    }

    // --- Create second fake SST file (2.txt)
    std::string sstFilePath2 = testDbName + "/2.txt";
    std::ofstream sstFile2(sstFilePath2);
    if (sstFile2.is_open()) {
        sstFile2 << "40 400\n";
        sstFile2 << "50 500\n";
        sstFile2 << "60 600\n";
        sstFile2.close();
    }

    // --- Create third fake SST file (3.txt)
    std::string sstFilePath3 = testDbName + "/3.txt";
    std::ofstream sstFile3(sstFilePath3);
    if (sstFile3.is_open()) {
        sstFile3 << "70 700\n";
        sstFile3 << "80 800\n";
        sstFile3 << "90 900\n";
        sstFile3.close();
    }

    // Create AVL instance pointing to fake DB directory
    AVL avl(5);
    avl.open_database(testDbName);

    int value;

    // --- Test 1: Key in first SST
    bool found1 = avl.get_from_sst(20, value);
    if (found1 && value == 200) {
        test_passed("SST Search Test 1: Key 20 found in SST 1.txt");
    } else {
        test_failed("SST Search Test 1: Key 20 not found or wrong value");
    }

    // --- Test 2: Key in second SST
    bool found2 = avl.get_from_sst(50, value);
    if (found2 && value == 500) {
        test_passed("SST Search Test 2: Key 50 found in SST 2.txt");
    } else {
        test_failed("SST Search Test 2: Key 50 not found or wrong value");
    }

    // --- Test 3: Key in third SST
    bool found3 = avl.get_from_sst(80, value);
    if (found3 && value == 800) {
        test_passed("SST Search Test 3: Key 80 found in SST 3.txt");
    } else {
        test_failed("SST Search Test 3: Key 80 not found or wrong value");
    }

    // --- Test 4: Key does not exist in any SST
    bool found4 = avl.get_from_sst(999, value);
    if (!found4) {
        test_passed("SST Search Test 4: Non-existent key correctly not found");
    } else {
        test_failed("SST Search Test 4: Non-existent key incorrectly found");
    }

    // Clean up
    cleanup_test_directory(testDbName);
}

void run_range_scan_with_sst_tests() {
    std::cout << "\n=== Range Scan with SST Tests ===" << std::endl;

    std::string testDbName = "test_range_scan_sst";
    cleanup_test_directory(testDbName);
    mkdir(testDbName.c_str(), 0777);

    // --- Create SST 1: multiple keys
    {
        std::ofstream sst(testDbName + "/1.txt");
        sst << "10 100\n";
        sst << "20 200\n";
        sst << "30 300\n";
        sst.close();
    }

    // --- Create SST 2: multiple keys
    {
        std::ofstream sst(testDbName + "/2.txt");
        sst << "40 400\n";
        sst << "50 500\n";
        sst << "60 600\n";
        sst.close();
    }

    // --- Create SST 3: one key
    {
        std::ofstream sst(testDbName + "/3.txt");
        sst << "70 700\n";
        sst.close();
    }

    // --- Create SST 4: empty file
    {
        std::ofstream sst(testDbName + "/4.txt");
        // no data written
        sst.close();
    }

    AVL avl(5);
    avl.open_database(testDbName);

    // --- Test 1: Range spanning SST 1 + SST 2
    auto result1 = avl.range_scan_with_sst(15, 55);
    std::cout << "Test 1: Range (15,55) → Expected keys: 20,30,40,50" << std::endl;
    if (result1.size() == 4 &&
        result1[0].first == 20 &&
        result1[1].first == 30 &&
        result1[2].first == 40 &&
        result1[3].first == 50) {
        test_passed("Range Scan Test 1: Spanning multiple SSTs");
    } else {
        test_failed("Range Scan Test 1: Spanning multiple SSTs");
    }

    // --- Test 2: Range inside SST 1
    auto result2 = avl.range_scan_with_sst(5, 25);
    std::cout << "Test 2: Range (5,25) → Expected keys: 10,20" << std::endl;
    if (result2.size() == 2 &&
        result2[0].first == 10 &&
        result2[1].first == 20) {
        test_passed("Range Scan Test 2: Inside single SST");
    } else {
        test_failed("Range Scan Test 2: Inside single SST");
    }

    // --- Test 3: Range inside SST with 1 element
    auto result3 = avl.range_scan_with_sst(65, 75);
    std::cout << "Test 3: Range (65,75) → Expected keys: 70" << std::endl;
    if (result3.size() == 1 && result3[0].first == 70) {
        test_passed("Range Scan Test 3: Single-element SST");
    } else {
        test_failed("Range Scan Test 3: Single-element SST");
    }

    // --- Test 4: Range inside empty SST (should return nothing)
    auto result4 = avl.range_scan_with_sst(200, 300);
    std::cout << "Test 4: Range (200,300) → Expected: 0 elements" << std::endl;
    if (result4.empty()) {
        test_passed("Range Scan Test 4: Empty SST returns no results");
    } else {
        test_failed("Range Scan Test 4: Empty SST incorrect results");
    }

    // --- Test 5: Range covering all SSTs
    auto result5 = avl.range_scan_with_sst(0, 1000);
    std::cout << "Test 5: Range (0,1000) → Expected all keys" << std::endl;
    if (result5.size() == 7 &&
        result5[0].first == 10 &&
        result5[1].first == 20 &&
        result5[2].first == 30 &&
        result5[3].first == 40 &&
        result5[4].first == 50 &&
        result5[5].first == 60 &&
        result5[6].first == 70) {
        test_passed("Range Scan Test 5: Full range across all SSTs");
    } else {
        test_failed("Range Scan Test 5: Full range across all SSTs");
    }

    avl.close_database();
    cleanup_test_directory(testDbName);
}

int main() {
    std::cout << "Running comprehensive test suite..." << std::endl;
    
    run_basic_tests();
    run_range_scan_tests();
    run_clear_memtable_tests();
    run_database_lifecycle_tests();
    run_auto_flush_tests();
    run_integration_tests();
    run_sst_search_tests();
    run_range_scan_with_sst_tests();
    
    print_test_summary();
    
    return (failed_tests == 0) ? 0 : 1;  // Return 0 if all tests passed, 1 if any failed
}