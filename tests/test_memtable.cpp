#include "../FileOperations.hpp"
#include "../AVL.hpp"
#include <iostream>

int memtable_tests_passed = 0;
int memtable_tests_failed = 0;

void memtable_test_passed(const std::string& test_name) {
    memtable_tests_passed++;
    std::cout << "[MEMTABLE] " << test_name << " PASSED!" << std::endl;
}

void memtable_test_failed(const std::string& test_name) {
    memtable_tests_failed++;
    std::cout << "[MEMTABLE] " << test_name << " FAILED!" << std::endl;
}

void test_memtable_interface() {
    std::cout << "\n--- Memtable Interface ---" << std::endl;
    
    Memtable_ds* memtable = new AVL(5);
    
    auto result = memtable->insert(10, 100);
    if (result != nullptr && memtable->get_size() == 1) {
        memtable_test_passed("Memtable Interface");
    } else {
        memtable_test_failed("Memtable Interface");
    }
    
    delete memtable;
}

void test_memtable_search() {
    std::cout << "\n--- Memtable Search ---" << std::endl;
    
    Memtable_ds* memtable = new AVL(10);
    
    // Insert multiple values
    memtable->insert(10, 100);
    memtable->insert(20, 200);
    memtable->insert(30, 300);
    
    int value;
    if (memtable->search(10, value) && value == 100 &&
        memtable->search(20, value) && value == 200 &&
        memtable->search(30, value) && value == 300) {
        memtable_test_passed("Memtable Search");
    } else {
        memtable_test_failed("Memtable Search");
    }
    
    delete memtable;
}

void test_memtable_range_operations() {
    std::cout << "\n--- Memtable Range Operations ---" << std::endl;
    
    Memtable_ds* memtable = new AVL(10);
    
    // Insert test data
    for (int i = 1; i <= 5; i++) {
        memtable->insert(i * 10, i * 100);
    }
    
    // Test range scan
    auto results = memtable->range_scan(20, 40);
    if (results.size() == 3 &&
        results[0].first == 20 && results[0].second == 200 &&
        results[1].first == 30 && results[1].second == 300 &&
        results[2].first == 40 && results[2].second == 400) {
        memtable_test_passed("Memtable Range Scan");
    } else {
        memtable_test_failed("Memtable Range Scan");
    }
    
    delete memtable;
}

void test_memtable_timing() {
    std::cout << "\n--- Memtable Timing Operations ---" << std::endl;
    
    Memtable_ds* memtable = new AVL(10);
    
    int time;
    auto result = memtable->timed_insert(15, 150, time);
    if (result != nullptr && time >= 0) {
        memtable_test_passed("Memtable Timed Insert");
    } else {
        memtable_test_failed("Memtable Timed Insert");
    }
    
    int value;
    bool found = memtable->timed_search(15, value, time);
    if (found && value == 150 && time >= 0) {
        memtable_test_passed("Memtable Timed Search");
    } else {
        memtable_test_failed("Memtable Timed Search");
    }
    
    delete memtable;
}

void test_memtable_inorder() {
    std::cout << "\n--- Memtable Inorder Traversal ---" << std::endl;
    
    Memtable_ds* memtable = new AVL(10);
    
    // Insert in random order
    memtable->insert(30, 300);
    memtable->insert(10, 100);
    memtable->insert(20, 200);
    
    auto inorder_values = memtable->inorder();
    if (inorder_values.size() == 3 &&
        inorder_values[0] == 100 &&  // Values should be in key order
        inorder_values[1] == 200 &&
        inorder_values[2] == 300) {
        memtable_test_passed("Memtable Inorder Traversal");
    } else {
        memtable_test_failed("Memtable Inorder Traversal");
    }
    
    delete memtable;
}

int run_memtable_tests() {
    std::cout << "\n========== MEMTABLE TESTS ==========" << std::endl;
    
    test_memtable_interface();
    test_memtable_search();
    test_memtable_range_operations();
    test_memtable_timing();
    test_memtable_inorder();
    
    std::cout << "\n=== MEMTABLE TEST SUMMARY ===" << std::endl;
    std::cout << "Memtable Tests Passed: " << memtable_tests_passed << std::endl;
    std::cout << "Memtable Tests Failed: " << memtable_tests_failed << std::endl;
    
    return memtable_tests_failed;
}

int main() {
    return run_memtable_tests();
}