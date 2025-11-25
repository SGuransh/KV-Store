#include "Database.hpp"
#include <iostream>
#include <cassert>

bool test_user_scenario() {
    std::cout << "\n========== USER SCENARIO TEST ==========\n" << std::endl;
    
    Database db(10);  // Use capacity 10 like in user's example
    
    // Clean up any existing test database
    system("rm -rf user_test");
    
    // Open database
    if (!db.open_database("user_test")) {
        std::cout << "[USER TEST] Database open FAILED!" << std::endl;
        return false;
    }
    
    std::cout << "Reproducing user's exact scenario..." << std::endl;
    
    // Recreate the exact scenario from user's example
    db.insert(1, 1);
    db.insert(2, 2);
    db.insert(3, 3);
    db.insert(4, 4);
    db.insert(5, 5);
    db.insert(6, 6);
    db.insert(7, 7);
    db.insert(8, 8);
    db.insert(9, 9);
    db.insert(10, 10);  // This should trigger flush to 1.txt
    
    // Update key 1 to value 2 (this goes to memtable then 2.txt)
    db.insert(1, 2);
    db.insert(2, 4);
    db.insert(11, 11);
    db.insert(12, 12);
    db.insert(13, 13);
    db.insert(14, 14);
    db.insert(15, 15);
    db.insert(16, 16);
    db.insert(17, 17);
    db.insert(18, 18);
    db.insert(19, 19);  // This should trigger flush to 2.txt
    db.insert(20, 20);
    
    std::cout << "\nTesting point search..." << std::endl;
    
    // Test key 1 - should return 2 (latest value), not 1 (old value)
    int value;
    bool found = db.search(1, value);
    if (!found) {
        std::cout << "[USER TEST] Key 1 not found FAILED!" << std::endl;
        return false;
    }
    
    std::cout << "Key 1 search result: " << value << " (expected: 2)" << std::endl;
    if (value != 2) {
        std::cout << "[USER TEST] Point search FAILED! Got " << value << " instead of 2" << std::endl;
        return false;
    }
    
    // Test key 2 - should return 4 (latest value), not 2 (old value)
    found = db.search(2, value);
    if (!found) {
        std::cout << "[USER TEST] Key 2 not found FAILED!" << std::endl;
        return false;
    }
    
    std::cout << "Key 2 search result: " << value << " (expected: 4)" << std::endl;
    if (value != 4) {
        std::cout << "[USER TEST] Point search FAILED! Got " << value << " instead of 4" << std::endl;
        return false;
    }
    
    std::cout << "\nTesting range scan..." << std::endl;
    
    // Test range scan including updated keys
    auto range_result = db.range_scan(1, 3);
    
    std::cout << "Range scan 1-3 results:" << std::endl;
    for (const auto& pair : range_result) {
        std::cout << "  Key: " << pair.first << " -> Value: " << pair.second << std::endl;
    }
    
    // Verify range scan returns latest values
    bool key1_correct = false, key2_correct = false, key3_correct = false;
    for (const auto& pair : range_result) {
        if (pair.first == 1 && pair.second == 2) key1_correct = true;
        if (pair.first == 2 && pair.second == 4) key2_correct = true;
        if (pair.first == 3 && pair.second == 3) key3_correct = true;
    }
    
    if (!key1_correct) {
        std::cout << "[USER TEST] Range scan FAILED! Key 1 does not have latest value 2" << std::endl;
        return false;
    }
    if (!key2_correct) {
        std::cout << "[USER TEST] Range scan FAILED! Key 2 does not have latest value 4" << std::endl;
        return false;
    }
    if (!key3_correct) {
        std::cout << "[USER TEST] Range scan FAILED! Key 3 does not have value 3" << std::endl;
        return false;
    }
    
    std::cout << "\n[USER TEST] PASSED! Both point search and range scan return latest values." << std::endl;
    
    db.close_database();
    
    // Clean up
    system("rm -rf user_test");
    
    return true;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "        User Scenario Test              " << std::endl;
    std::cout << "========================================" << std::endl;
    
    bool passed = test_user_scenario();
    
    std::cout << "\n========================================" << std::endl;
    if (passed) {
        std::cout << "✅ USER SCENARIO TEST PASSED" << std::endl;
        std::cout << "Search order issue has been resolved!" << std::endl;
    } else {
        std::cout << "❌ USER SCENARIO TEST FAILED" << std::endl;
        std::cout << "Search order issue still exists!" << std::endl;
    }
    std::cout << "========================================" << std::endl;
    
    return passed ? 0 : 1;
}