#include "../FileOperations.cpp"
#include "../AVL.cpp"
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

int run_memtable_tests() {
    std::cout << "\n========== MEMTABLE TESTS ==========" << std::endl;
    
    Memtable_ds* memtable = new AVL(5);
    
    auto result = memtable->insert(10, 100);
    if (result != nullptr && memtable->get_size() == 1) {
        memtable_test_passed("Memtable Interface");
    } else {
        memtable_test_failed("Memtable Interface");
    }
    
    int value;
    if (memtable->search(10, value) && value == 100) {
        memtable_test_passed("Memtable Search");
    } else {
        memtable_test_failed("Memtable Search");
    }
    
    delete memtable;
    
    std::cout << "\n=== MEMTABLE TEST SUMMARY ===" << std::endl;
    std::cout << "Memtable Tests Passed: " << memtable_tests_passed << std::endl;
    std::cout << "Memtable Tests Failed: " << memtable_tests_failed << std::endl;
    
    return memtable_tests_failed;
}

int main() {
    return run_memtable_tests();
}