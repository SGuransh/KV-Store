#include "../FileOperations.hpp"
#include "../MemtableFactory.hpp"
#include "../Memtable_ds.hpp"
#include <iostream>

int avl_tests_passed = 0;
int avl_tests_failed = 0;

void avl_test_passed(const std::string& test_name) {
    avl_tests_passed++;
    std::cout << "[AVL] " << test_name << " PASSED!" << std::endl;
}

void avl_test_failed(const std::string& test_name) {
    avl_tests_failed++;
    std::cout << "[AVL] " << test_name << " FAILED!" << std::endl;
}

void test_basic_avl_operations() {
    std::cout << "\n--- Basic AVL Operations ---" << std::endl;
    
    std::unique_ptr<Memtable_ds> avl = create_memtable(MemtableType::AVL, 10);
    avl->insert(30, 300);
    avl->insert(20, 200);
    avl->insert(40, 400);

    std::cout << "Inorder Traversal: ";
    for (int key : avl->inorder()) {
        std::cout << key << " ";
    }
    std::cout << std::endl;

    std::cout << "AVL Tree Size: " << avl->get_size() << std::endl;

    if (avl->get_size() == 3) {
        avl_test_passed("AVL Basic Operations");
    } else {
        avl_test_failed("AVL Basic Operations");
    }
}

void test_avl_search() {
    std::cout << "\n--- AVL Search Operations ---" << std::endl;
    
    std::unique_ptr<Memtable_ds> avl = create_memtable(MemtableType::AVL, 10);
    
    // Insert test data
    avl->insert(50, 500);
    avl->insert(30, 300);
    avl->insert(70, 700);
    avl->insert(20, 200);
    avl->insert(40, 400);
    
    int value;
    
    // Test successful searches
    if (avl->search(30, value) && value == 300) {
        avl_test_passed("AVL Search Found");
    } else {
        avl_test_failed("AVL Search Found");
    }
    
    // Test unsuccessful search
    if (!avl->search(99, value)) {
        avl_test_passed("AVL Search Not Found");
    } else {
        avl_test_failed("AVL Search Not Found");
    }
    
    // Test edge cases
    if (avl->search(20, value) && value == 200 &&
        avl->search(70, value) && value == 700) {
        avl_test_passed("AVL Search Edge Cases");
    } else {
        avl_test_failed("AVL Search Edge Cases");
    }
}

void test_avl_range_scan() {
    std::cout << "\n--- AVL Range Scan Operations ---" << std::endl;
    
    std::unique_ptr<Memtable_ds> avl = create_memtable(MemtableType::AVL, 10);
    
    // Insert test data
    for (int i = 10; i <= 50; i += 10) {
        avl->insert(i, i * 10);
    }
    
    // Test inclusive range scan
    auto result = avl->range_scan(20, 40);
    if (result.size() == 3 &&
        result[0].first == 20 && result[0].second == 200 &&
        result[1].first == 30 && result[1].second == 300 &&
        result[2].first == 40 && result[2].second == 400) {
        avl_test_passed("AVL Range Scan Inclusive");
    } else {
        avl_test_failed("AVL Range Scan Inclusive");
    }
    
    // Test single key range
    auto single_result = avl->range_scan(30, 30);
    if (single_result.size() == 1 &&
        single_result[0].first == 30 && single_result[0].second == 300) {
        avl_test_passed("AVL Range Scan Single Key");
    } else {
        avl_test_failed("AVL Range Scan Single Key");
    }
    
    // Test empty range
    auto empty_result = avl->range_scan(35, 35);
    if (empty_result.size() == 0) {
        avl_test_passed("AVL Range Scan Empty");
    } else {
        avl_test_failed("AVL Range Scan Empty");
    }
    
    // Test full range
    auto full_result = avl->range_scan(10, 50);
    if (full_result.size() == 5) {
        avl_test_passed("AVL Range Scan Full");
    } else {
        avl_test_failed("AVL Range Scan Full");
    }
}



void test_avl_capacity() {
    std::cout << "\n--- AVL Capacity Handling ---" << std::endl;
    
    std::unique_ptr<Memtable_ds> avl = create_memtable(MemtableType::AVL, 3);
    
    // Fill to capacity
    avl->insert(10, 100);
    avl->insert(20, 200);
    avl->insert(30, 300);
    
    if (avl->get_size() == 3 && avl->get_max_elements() == 3) {
        avl_test_passed("AVL Capacity Management");
    } else {
        avl_test_failed("AVL Capacity Management");
    }
}

int run_avl_tests() {
    std::cout << "\n========== AVL TESTS ==========" << std::endl;
    
    test_basic_avl_operations();
    test_avl_search();
    test_avl_range_scan();
    test_avl_capacity();
    
    std::cout << "\n=== AVL TEST SUMMARY ===" << std::endl;
    std::cout << "AVL Tests Passed: " << avl_tests_passed << std::endl;
    std::cout << "AVL Tests Failed: " << avl_tests_failed << std::endl;
    
    return avl_tests_failed;
}

int main() {
    return run_avl_tests();
}