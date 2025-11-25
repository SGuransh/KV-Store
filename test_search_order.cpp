#include "Database.hpp"
#include <iostream>
#include <cassert>

bool test_search_order() {
    std::cout << "\n========== SEARCH ORDER TEST ==========\n" << std::endl;
    
    Database db(3);  // Small capacity to force multiple SST flushes
    
    // Clean up any existing test database
    system("rm -rf test_search_order");
    
    // Open database
    if (!db.open_database("test_search_order")) {
        std::cout << "[SEARCH ORDER] Database open FAILED!" << std::endl;
        return false;
    }
    
    std::cout << "--- Testing SST Search Order ---" << std::endl;
    
    // Insert initial data to fill first SST
    std::cout << "Phase 1: Inserting initial data..." << std::endl;
    db.insert(1, 10);
    db.insert(2, 20);
    db.insert(3, 30);  // This should trigger flush to 1.txt
    
    // Insert more data to fill second SST
    std::cout << "Phase 2: Inserting more data..." << std::endl;
    db.insert(4, 40);
    db.insert(5, 50);
    db.insert(6, 60);  // This should trigger flush to 2.txt
    
    // Now update key 1 with a new value - this should go to memtable
    std::cout << "Phase 3: Updating existing key..." << std::endl;
    db.insert(1, 100);  // Updated value: 1 -> 100 (should be in memtable)
    
    // Flush memtable to create 3.txt
    db.insert(7, 70);
    db.insert(8, 80);  // This should trigger flush to 3.txt with key 1 having value 100
    
    // Force close and reopen to ensure all data is in SST files (no memtable)
    std::cout << "Phase 3b: Closing and reopening to force all data to SST..." << std::endl;
    db.close_database();
    if (!db.open_database("test_search_order")) {
        std::cout << "[SEARCH ORDER] Database reopen FAILED!" << std::endl;
        return false;
    }
    
    // Search for key 1 - should return 100 (from newest SST), not 10 (from oldest SST)
    std::cout << "Phase 4: Searching for updated key..." << std::endl;
    int found_value;
    bool found = db.search(1, found_value);
    
    if (!found) {
        std::cout << "[SEARCH ORDER] Key 1 not found FAILED!" << std::endl;
        return false;
    }
    
    std::cout << "Key 1 found with value: " << found_value << std::endl;
    
    if (found_value != 100) {
        std::cout << "[SEARCH ORDER] FAILED! Expected value 100, got " << found_value << std::endl;
        std::cout << "This indicates SST files are being searched in wrong order!" << std::endl;
        return false;
    }
    
    std::cout << "[SEARCH ORDER] PASSED! Correctly returned latest value." << std::endl;
    
    // Test range scan as well
    std::cout << "--- Testing Range Scan Order ---" << std::endl;
    auto scan_result = db.range_scan(1, 1);  // Scan for just key 1
    
    if (scan_result.empty()) {
        std::cout << "[RANGE SCAN ORDER] Range scan failed to find key 1!" << std::endl;
        return false;
    }
    
    // Should contain only one entry with value 100
    bool correct_scan = false;
    for (const auto& pair : scan_result) {
        if (pair.first == 1 && pair.second == 100) {
            correct_scan = true;
            break;
        }
    }
    
    if (!correct_scan) {
        std::cout << "[RANGE SCAN ORDER] FAILED! Range scan did not return correct latest value." << std::endl;
        std::cout << "Scan results for key 1:" << std::endl;
        for (const auto& pair : scan_result) {
            if (pair.first == 1) {
                std::cout << "  Key: " << pair.first << ", Value: " << pair.second << std::endl;
            }
        }
        return false;
    }
    
    std::cout << "[RANGE SCAN ORDER] PASSED! Range scan returned latest value." << std::endl;
    
    db.close_database();
    
    // Clean up
    system("rm -rf test_search_order");
    
    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "      SST Search Order Test Suite      " << std::endl;
    std::cout << "========================================" << std::endl;
    
    bool all_passed = true;
    
    if (!test_search_order()) {
        all_passed = false;
    }
    
    std::cout << "\n========================================" << std::endl;
    if (all_passed) {
        std::cout << "✅ ALL TESTS PASSED" << std::endl;
    } else {
        std::cout << "❌ SOME TESTS FAILED" << std::endl;
    }
    std::cout << "========================================" << std::endl;
    
    return all_passed ? 0 : 1;
}