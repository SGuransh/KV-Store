#include "../HashTable.hpp"
#include "../PageID.hpp"
#include "../Page.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <vector>

int hashtable_tests_passed = 0;
int hashtable_tests_failed = 0;

void hashtable_test_passed(const std::string& test_name) {
    hashtable_tests_passed++;
    std::cout << "[HASHTABLE] " << test_name << " PASSED!" << std::endl;
}

void hashtable_test_failed(const std::string& test_name) {
    hashtable_tests_failed++;
    std::cout << "[HASHTABLE] " << test_name << " FAILED!" << std::endl;
}

void test_basic_hashtable_operations() {
    std::cout << "\n--- Basic HashTable Operations ---" << std::endl;
    
    HashTable table(7);  // Small table for testing
    
    // Create test pages
    PageID id1("file1.db", 0);
    PageID id2("file1.db", 4096);
    PageID id3("file2.db", 0);
    
    Page page1, page2, page3;
    strcpy(page1.getData(), "Test data 1");
    strcpy(page2.getData(), "Test data 2");
    strcpy(page3.getData(), "Test data 3");
    
    // Test insertion
    if (table.insert(id1, page1) && table.insert(id2, page2) && table.insert(id3, page3)) {
        hashtable_test_passed("HashTable Basic Insert");
    } else {
        hashtable_test_failed("HashTable Basic Insert");
    }
    
    // Test size tracking
    if (table.getCurrentSize() == 3) {
        hashtable_test_passed("HashTable Size Tracking");
    } else {
        hashtable_test_failed("HashTable Size Tracking");
    }
    
    // Test retrieval
    Page* found1 = table.find(id1);
    Page* found2 = table.find(id2);
    Page* found3 = table.find(id3);
    
    if (found1 != nullptr && strcmp(found1->getData(), "Test data 1") == 0 &&
        found2 != nullptr && strcmp(found2->getData(), "Test data 2") == 0 &&
        found3 != nullptr && strcmp(found3->getData(), "Test data 3") == 0) {
        hashtable_test_passed("HashTable Basic Find");
    } else {
        hashtable_test_failed("HashTable Basic Find");
    }
}

void test_hashtable_collisions() {
    std::cout << "\n--- HashTable Collision Handling ---" << std::endl;
    
    HashTable table(3);  // Very small table to force collisions
    
    // Insert multiple pages that will likely collide
    std::vector<PageID> ids;
    std::vector<Page> pages;
    
    for (int i = 0; i < 10; i++) {
        ids.emplace_back("test" + std::to_string(i) + ".db", i * 4096);
        pages.emplace_back();
        strcpy(pages[i].getData(), ("Data " + std::to_string(i)).c_str());
        table.insert(ids[i], pages[i]);
    }
    
    if (table.getCurrentSize() == 10) {
        hashtable_test_passed("HashTable Collision Insert");
    } else {
        hashtable_test_failed("HashTable Collision Insert");
    }
    
    // Verify all pages can be found despite collisions
    bool all_found = true;
    for (int i = 0; i < 10; i++) {
        Page* found = table.find(ids[i]);
        if (found == nullptr || strcmp(found->getData(), ("Data " + std::to_string(i)).c_str()) != 0) {
            all_found = false;
            break;
        }
    }
    
    if (all_found) {
        hashtable_test_passed("HashTable Collision Find");
    } else {
        hashtable_test_failed("HashTable Collision Find");
    }
}

void test_hashtable_updates() {
    std::cout << "\n--- HashTable Update Operations ---" << std::endl;
    
    HashTable table(7);
    
    PageID id("update_test.db", 0);
    Page original_page, updated_page;
    strcpy(original_page.getData(), "Original data");
    strcpy(updated_page.getData(), "Updated data");
    
    // Insert original
    table.insert(id, original_page);
    
    // Update with same PageID
    table.insert(id, updated_page);
    
    // Size should remain 1 (update, not new insert)
    if (table.getCurrentSize() == 1) {
        hashtable_test_passed("HashTable Update Size");
    } else {
        hashtable_test_failed("HashTable Update Size");
    }
    
    // Verify updated data
    Page* found = table.find(id);
    if (found != nullptr && strcmp(found->getData(), "Updated data") == 0) {
        hashtable_test_passed("HashTable Update Data");
    } else {
        hashtable_test_failed("HashTable Update Data");
    }
}

void test_hashtable_removal() {
    std::cout << "\n--- HashTable Remove Operations ---" << std::endl;
    
    HashTable table(7);
    
    PageID id1("remove1.db", 0);
    PageID id2("remove2.db", 0);
    PageID id3("remove3.db", 0);
    
    Page page1, page2, page3;
    strcpy(page1.getData(), "Remove test 1");
    strcpy(page2.getData(), "Remove test 2");
    strcpy(page3.getData(), "Remove test 3");
    
    // Insert pages
    table.insert(id1, page1);
    table.insert(id2, page2);
    table.insert(id3, page3);
    
    // Remove middle page
    if (table.remove(id2)) {
        hashtable_test_passed("HashTable Remove Success");
    } else {
        hashtable_test_failed("HashTable Remove Success");
    }
    
    // Verify size decreased
    if (table.getCurrentSize() == 2) {
        hashtable_test_passed("HashTable Remove Size Update");
    } else {
        hashtable_test_failed("HashTable Remove Size Update");
    }
    
    // Verify removed page not found
    if (table.find(id2) == nullptr) {
        hashtable_test_passed("HashTable Remove Verification");
    } else {
        hashtable_test_failed("HashTable Remove Verification");
    }
    
    // Verify other pages still exist
    if (table.find(id1) != nullptr && table.find(id3) != nullptr) {
        hashtable_test_passed("HashTable Remove Integrity");
    } else {
        hashtable_test_failed("HashTable Remove Integrity");
    }
    
    // Test removing non-existent page
    PageID nonexistent("nonexistent.db", 0);
    if (!table.remove(nonexistent)) {
        hashtable_test_passed("HashTable Remove Nonexistent");
    } else {
        hashtable_test_failed("HashTable Remove Nonexistent");
    }
}

void test_hashtable_hash_distribution() {
    std::cout << "\n--- HashTable Hash Distribution ---" << std::endl;
    
    HashTable table(11);  // Prime number for better distribution
    
    // Insert pages with different patterns
    for (int i = 0; i < 20; i++) {
        PageID id("dist_test_" + std::to_string(i) + ".db", i * 4096);
        Page page;
        strcpy(page.getData(), ("Distribution test " + std::to_string(i)).c_str());
        table.insert(id, page);
    }
    
    // Get distribution statistics
    std::size_t maxChainLength, emptyBuckets;
    double avgChainLength;
    table.getStatistics(maxChainLength, avgChainLength, emptyBuckets);
    
    // Check that distribution is reasonable (no extremely long chains)
    if (maxChainLength <= 5) {  // Reasonable for 20 items in 11 buckets
        hashtable_test_passed("HashTable Distribution Max Chain");
    } else {
        hashtable_test_failed("HashTable Distribution Max Chain");
    }
    
    // Check load factor
    double loadFactor = table.getLoadFactor();
    if (loadFactor > 1.5 && loadFactor < 2.5) {  // 20/11 ≈ 1.82
        hashtable_test_passed("HashTable Load Factor");
    } else {
        hashtable_test_failed("HashTable Load Factor");
    }
}

void test_hashtable_boundary_conditions() {
    std::cout << "\n--- HashTable Boundary Conditions ---" << std::endl;
    
    HashTable table(5);
    
    // Test empty table operations
    PageID nonexistent("empty_test.db", 0);
    if (table.find(nonexistent) == nullptr && !table.remove(nonexistent)) {
        hashtable_test_passed("HashTable Empty Operations");
    } else {
        hashtable_test_failed("HashTable Empty Operations");
    }
    
    // Test single element
    PageID single("single.db", 0);
    Page singlePage;
    strcpy(singlePage.getData(), "Single element");
    
    table.insert(single, singlePage);
    
    if (table.getCurrentSize() == 1 && !table.isEmpty()) {
        hashtable_test_passed("HashTable Single Element");
    } else {
        hashtable_test_failed("HashTable Single Element");
    }
    
    // Remove single element
    table.remove(single);
    
    if (table.getCurrentSize() == 0 && table.isEmpty()) {
        hashtable_test_passed("HashTable Empty After Remove");
    } else {
        hashtable_test_failed("HashTable Empty After Remove");
    }
}

int run_hashtable_tests() {
    std::cout << "\n========== HASHTABLE TESTS ==========" << std::endl;
    
    test_basic_hashtable_operations();
    test_hashtable_collisions();
    test_hashtable_updates();
    test_hashtable_removal();
    test_hashtable_hash_distribution();
    test_hashtable_boundary_conditions();
    
    std::cout << "\n=== HASHTABLE TEST SUMMARY ===" << std::endl;
    std::cout << "HashTable Tests Passed: " << hashtable_tests_passed << std::endl;
    std::cout << "HashTable Tests Failed: " << hashtable_tests_failed << std::endl;
    
    return hashtable_tests_failed;
}

int main() {
    return run_hashtable_tests();
}