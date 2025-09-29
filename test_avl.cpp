#include "FileOperations.cpp"
#include "AVL.cpp"
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

int run_avl_tests() {
    std::cout << "\n========== AVL TESTS ==========" << std::endl;
    
    AVL avl(10);
    avl.insert(30, 300);
    avl.insert(20, 200);
    avl.insert(40, 400);
    
    if (avl.get_size() == 3) {
        avl_test_passed("AVL Basic Operations");
    } else {
        avl_test_failed("AVL Basic Operations");
    }
    
    int value;
    if (avl.search(20, value) && value == 200) {
        avl_test_passed("AVL Search");
    } else {
        avl_test_failed("AVL Search");
    }
    
    auto result = avl.range_scan(15, 45);
    if (result.size() == 3) {
        avl_test_passed("AVL Range Scan");
    } else {
        avl_test_failed("AVL Range Scan");
    }
    
    std::cout << "\n=== AVL TEST SUMMARY ===" << std::endl;
    std::cout << "AVL Tests Passed: " << avl_tests_passed << std::endl;
    std::cout << "AVL Tests Failed: " << avl_tests_failed << std::endl;
    
    return avl_tests_failed;
}

int main() {
    return run_avl_tests();
}