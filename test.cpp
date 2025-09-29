#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "         KV-Store Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    int total_failures = 0;
    
    // Run individual test executables
    std::cout << "\n--- Running AVL Tests ---" << std::endl;
    int avl_result = system("./test_avl");
    if (avl_result != 0) total_failures++;
    
    std::cout << "\n--- Running Memtable Tests ---" << std::endl;
    int memtable_result = system("./test_memtable");
    if (memtable_result != 0) total_failures++;
    
    std::cout << "\n--- Running Database Tests ---" << std::endl;
    int database_result = system("./test_database");
    if (database_result != 0) total_failures++;
    
    // Print overall summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "         OVERALL TEST SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    
    if (total_failures == 0) {
        std::cout << "🎉 ALL TESTS PASSED! 🎉" << std::endl;
        std::cout << "Total Failed Test Suites: 0" << std::endl;
    } else {
        std::cout << "❌ SOME TESTS FAILED ❌" << std::endl;
        std::cout << "Total Failed Test Suites: " << total_failures << std::endl;
    }
    
    std::cout << "========================================" << std::endl;
    
    return (total_failures == 0) ? 0 : 1;
}