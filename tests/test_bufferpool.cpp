#include "../BufferPool/BufferPool.hpp"
#include "../BufferPool/ClockEvictionPolicy.hpp"
#include "../BufferPool/PageID.hpp"
#include "../BufferPool/Page.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <memory>
#include <vector>

int bufferpool_tests_passed = 0;
int bufferpool_tests_failed = 0;

void bufferpool_test_passed(const std::string& test_name) {
    bufferpool_tests_passed++;
    std::cout << "[BUFFERPOOL] " << test_name << " PASSED!" << std::endl;
}

void bufferpool_test_failed(const std::string& test_name) {
    bufferpool_tests_failed++;
    std::cout << "[BUFFERPOOL] " << test_name << " FAILED!" << std::endl;
}

void test_bufferpool_basic_operations() {
    std::cout << "\n--- BufferPool Basic Operations ---" << std::endl;
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    BufferPool pool(3, std::move(policy));  // Small buffer for testing
    
    // Test initial state
    if (pool.isEmpty() && pool.getCurrentSize() == 0 && pool.getBufferSize() == 3) {
        bufferpool_test_passed("BufferPool Initial State");
    } else {
        bufferpool_test_failed("BufferPool Initial State");
    }
    
    // Create test pages
    PageID id1("file1.db", 0);
    PageID id2("file1.db", 4096);
    PageID id3("file2.db", 0);
    
    Page page1, page2, page3;
    strcpy(page1.getData(), "Buffer test 1");
    strcpy(page2.getData(), "Buffer test 2");
    strcpy(page3.getData(), "Buffer test 3");
    
    // Test insertion
    if (pool.putPage(id1, page1) && pool.putPage(id2, page2) && pool.putPage(id3, page3)) {
        bufferpool_test_passed("BufferPool Basic Insert");
    } else {
        bufferpool_test_failed("BufferPool Basic Insert");
    }
    
    // Test size tracking
    if (pool.getCurrentSize() == 3 && pool.isAtCapacity()) {
        bufferpool_test_passed("BufferPool Size Tracking");
    } else {
        bufferpool_test_failed("BufferPool Size Tracking");
    }
    
    // Test retrieval
    Page* found1 = pool.getPage(id1);
    Page* found2 = pool.getPage(id2);
    Page* found3 = pool.getPage(id3);
    
    if (found1 != nullptr && strcmp(found1->getData(), "Buffer test 1") == 0 &&
        found2 != nullptr && strcmp(found2->getData(), "Buffer test 2") == 0 &&
        found3 != nullptr && strcmp(found3->getData(), "Buffer test 3") == 0) {
        bufferpool_test_passed("BufferPool Basic Retrieval");
    } else {
        bufferpool_test_failed("BufferPool Basic Retrieval");
    }
}

void test_bufferpool_eviction() {
    std::cout << "\n--- BufferPool Eviction Management ---" << std::endl;
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    BufferPool pool(2, std::move(policy));  // Very small buffer to force eviction
    
    // Fill buffer to capacity
    PageID id1("evict1.db", 0);
    PageID id2("evict2.db", 0);
    Page page1, page2;
    strcpy(page1.getData(), "Evict test 1");
    strcpy(page2.getData(), "Evict test 2");
    
    pool.putPage(id1, page1);
    pool.putPage(id2, page2);
    
    if (pool.isAtCapacity()) {
        bufferpool_test_passed("BufferPool Fill to Capacity");
    } else {
        bufferpool_test_failed("BufferPool Fill to Capacity");
    }
    
    // Insert third page - should trigger eviction
    PageID id3("evict3.db", 0);
    Page page3;
    strcpy(page3.getData(), "Evict test 3");
    
    if (pool.putPage(id3, page3)) {
        bufferpool_test_passed("BufferPool Eviction Insert");
    } else {
        bufferpool_test_failed("BufferPool Eviction Insert");
    }
    
    // Buffer should still be at capacity
    if (pool.getCurrentSize() == 2 && pool.isAtCapacity()) {
        bufferpool_test_passed("BufferPool Size After Eviction");
    } else {
        bufferpool_test_failed("BufferPool Size After Eviction");
    }
    
    // New page should be findable
    Page* found3 = pool.getPage(id3);
    if (found3 != nullptr && strcmp(found3->getData(), "Evict test 3") == 0) {
        bufferpool_test_passed("BufferPool New Page After Eviction");
    } else {
        bufferpool_test_failed("BufferPool New Page After Eviction");
    }
}

void test_bufferpool_page_updates() {
    std::cout << "\n--- BufferPool Page Updates ---" << std::endl;
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    BufferPool pool(3, std::move(policy));
    
    PageID id("update.db", 0);
    Page originalPage, updatedPage;
    strcpy(originalPage.getData(), "Original buffer data");
    strcpy(updatedPage.getData(), "Updated buffer data");
    
    // Insert original page
    pool.putPage(id, originalPage);
    
    // Update with same PageID
    pool.putPage(id, updatedPage);
    
    // Size should remain 1 (update, not new insert)
    if (pool.getCurrentSize() == 1) {
        bufferpool_test_passed("BufferPool Update Size");
    } else {
        bufferpool_test_failed("BufferPool Update Size");
    }
    
    // Verify updated data
    Page* found = pool.getPage(id);
    if (found != nullptr && strcmp(found->getData(), "Updated buffer data") == 0) {
        bufferpool_test_passed("BufferPool Update Data");
    } else {
        bufferpool_test_failed("BufferPool Update Data");
    }
}

void test_bufferpool_removal() {
    std::cout << "\n--- BufferPool Page Removal ---" << std::endl;
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    BufferPool pool(3, std::move(policy));
    
    PageID id1("remove1.db", 0);
    PageID id2("remove2.db", 0);
    PageID id3("remove3.db", 0);
    
    Page page1, page2, page3;
    strcpy(page1.getData(), "Remove buffer 1");
    strcpy(page2.getData(), "Remove buffer 2");
    strcpy(page3.getData(), "Remove buffer 3");
    
    // Insert pages
    pool.putPage(id1, page1);
    pool.putPage(id2, page2);
    pool.putPage(id3, page3);
    
    // Remove middle page
    if (pool.removePage(id2)) {
        bufferpool_test_passed("BufferPool Remove Success");
    } else {
        bufferpool_test_failed("BufferPool Remove Success");
    }
    
    // Verify size decreased
    if (pool.getCurrentSize() == 2) {
        bufferpool_test_passed("BufferPool Remove Size Update");
    } else {
        bufferpool_test_failed("BufferPool Remove Size Update");
    }
    
    // Verify removed page not found
    if (pool.getPage(id2) == nullptr) {
        bufferpool_test_passed("BufferPool Remove Verification");
    } else {
        bufferpool_test_failed("BufferPool Remove Verification");
    }
    
    // Verify other pages still exist
    if (pool.getPage(id1) != nullptr && pool.getPage(id3) != nullptr) {
        bufferpool_test_passed("BufferPool Remove Integrity");
    } else {
        bufferpool_test_failed("BufferPool Remove Integrity");
    }
}

void test_bufferpool_eviction_policy_integration() {
    std::cout << "\n--- BufferPool Eviction Policy Integration ---" << std::endl;
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    BufferPool pool(3, std::move(policy));
    
    // Insert pages and access them in specific pattern
    PageID id1("policy1.db", 0);
    PageID id2("policy2.db", 0);
    PageID id3("policy3.db", 0);
    
    Page page1, page2, page3;
    strcpy(page1.getData(), "Policy test 1");
    strcpy(page2.getData(), "Policy test 2");
    strcpy(page3.getData(), "Policy test 3");
    
    pool.putPage(id1, page1);
    pool.putPage(id2, page2);
    pool.putPage(id3, page3);
    
    // Access pages to set reference bits
    pool.getPage(id1);  // Access page 1
    pool.getPage(id3);  // Access page 3
    // Don't access page 2
    
    // Insert fourth page - should evict one of the pages
    PageID id4("policy4.db", 0);
    Page page4;
    strcpy(page4.getData(), "Policy test 4");
    
    pool.putPage(id4, page4);
    
    // Check that exactly one page was evicted and the new page is present
    int pagesFound = 0;
    if (pool.getPage(id1) != nullptr) pagesFound++;
    if (pool.getPage(id2) != nullptr) pagesFound++;
    if (pool.getPage(id3) != nullptr) pagesFound++;
    if (pool.getPage(id4) != nullptr) pagesFound++;
    
    if (pagesFound == 3 && pool.getPage(id4) != nullptr) {
        bufferpool_test_passed("BufferPool Eviction Policy Integration");
    } else {
        bufferpool_test_failed("BufferPool Eviction Policy Integration");
    }
}

void test_bufferpool_capacity_management() {
    std::cout << "\n--- BufferPool Capacity Management ---" << std::endl;
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    BufferPool pool(5, std::move(policy));
    
    // Fill buffer gradually and test capacity checks
    std::vector<PageID> ids;
    std::vector<Page> pages;
    
    for (int i = 0; i < 5; i++) {
        ids.emplace_back("capacity" + std::to_string(i) + ".db", i * 4096);
        pages.emplace_back();
        strcpy(pages[i].getData(), ("Capacity test " + std::to_string(i)).c_str());
        
        pool.putPage(ids[i], pages[i]);
        
        if (pool.getCurrentSize() == static_cast<std::size_t>(i + 1)) {
            // Size tracking is correct
        } else {
            bufferpool_test_failed("BufferPool Capacity Size Tracking");
            return;
        }
    }
    
    if (pool.isAtCapacity() && !pool.isEmpty()) {
        bufferpool_test_passed("BufferPool Capacity Management");
    } else {
        bufferpool_test_failed("BufferPool Capacity Management");
    }
    
    // Test load factor
    double loadFactor = pool.getLoadFactor();
    if (loadFactor > 0.0) {  // Should have some load
        bufferpool_test_passed("BufferPool Load Factor");
    } else {
        bufferpool_test_failed("BufferPool Load Factor");
    }
}

void test_bufferpool_boundary_conditions() {
    std::cout << "\n--- BufferPool Boundary Conditions ---" << std::endl;
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    BufferPool pool(1, std::move(policy));  // Single page buffer
    
    PageID id1("boundary1.db", 0);
    PageID id2("boundary2.db", 0);
    Page page1, page2;
    strcpy(page1.getData(), "Boundary test 1");
    strcpy(page2.getData(), "Boundary test 2");
    
    // Insert first page
    pool.putPage(id1, page1);
    
    if (pool.getCurrentSize() == 1 && pool.isAtCapacity()) {
        bufferpool_test_passed("BufferPool Single Page Buffer");
    } else {
        bufferpool_test_failed("BufferPool Single Page Buffer");
    }
    
    // Insert second page - should evict first
    pool.putPage(id2, page2);
    
    if (pool.getCurrentSize() == 1 && 
        pool.getPage(id1) == nullptr && 
        pool.getPage(id2) != nullptr) {
        bufferpool_test_passed("BufferPool Single Page Eviction");
    } else {
        bufferpool_test_failed("BufferPool Single Page Eviction");
    }
    
    // Test removal from single page buffer
    pool.removePage(id2);
    
    if (pool.isEmpty() && pool.getCurrentSize() == 0) {
        bufferpool_test_passed("BufferPool Empty After Single Remove");
    } else {
        bufferpool_test_failed("BufferPool Empty After Single Remove");
    }
}

int run_bufferpool_tests() {
    std::cout << "\n========== BUFFERPOOL TESTS ==========" << std::endl;
    
    test_bufferpool_basic_operations();
    test_bufferpool_eviction();
    test_bufferpool_page_updates();
    test_bufferpool_removal();
    test_bufferpool_eviction_policy_integration();
    test_bufferpool_capacity_management();
    test_bufferpool_boundary_conditions();
    
    std::cout << "\n=== BUFFERPOOL TEST SUMMARY ===" << std::endl;
    std::cout << "BufferPool Tests Passed: " << bufferpool_tests_passed << std::endl;
    std::cout << "BufferPool Tests Failed: " << bufferpool_tests_failed << std::endl;
    
    return bufferpool_tests_failed;
}

int main() {
    return run_bufferpool_tests();
}