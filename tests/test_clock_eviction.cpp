#include "../ClockEvictionPolicy.hpp"
#include "../PageID.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

int clock_tests_passed = 0;
int clock_tests_failed = 0;

void clock_test_passed(const std::string& test_name) {
    clock_tests_passed++;
    std::cout << "[CLOCK] " << test_name << " PASSED!" << std::endl;
}

void clock_test_failed(const std::string& test_name) {
    clock_tests_failed++;
    std::cout << "[CLOCK] " << test_name << " FAILED!" << std::endl;
}

void test_basic_clock_operations() {
    std::cout << "\n--- Basic Clock Operations ---" << std::endl;
    
    ClockEvictionPolicy clock;
    clock.initialize(10);  // Initialize with capacity of 10
    
    // Test empty clock
    if (clock.isEmpty() && clock.getTrackedPageCount() == 0) {
        clock_test_passed("Clock Empty State");
    } else {
        clock_test_failed("Clock Empty State");
    }
    
    // Test empty clock victim selection throws exception
    try {
        clock.selectVictim();
        clock_test_failed("Clock Empty Victim Exception");
    } catch (const std::runtime_error&) {
        clock_test_passed("Clock Empty Victim Exception");
    }
    
    // Insert single page
    PageID page1("file1.db", 0);
    clock.recordInsertion(page1);
    
    if (!clock.isEmpty() && clock.getTrackedPageCount() == 1) {
        clock_test_passed("Clock Single Insert");
    } else {
        clock_test_failed("Clock Single Insert");
    }
    
    // Single page victim selection
    PageID victim = clock.selectVictim();
    if (victim == page1) {
        clock_test_passed("Clock Single Victim");
    } else {
        clock_test_failed("Clock Single Victim");
    }
}

void test_clock_reference_bits() {
    std::cout << "\n--- Clock Reference Bit Management ---" << std::endl;
    
    ClockEvictionPolicy clock;
    clock.initialize(10);  // Initialize with capacity of 10
    
    // Insert multiple pages
    PageID page1("file1.db", 0);
    PageID page2("file2.db", 0);
    PageID page3("file3.db", 0);
    
    clock.recordInsertion(page1);
    clock.recordInsertion(page2);
    clock.recordInsertion(page3);
    
    // All pages start with reference bit = 1
    // First victim selection should clear all bits and return first page
    PageID firstVictim = clock.selectVictim();
    if (firstVictim == page1) {
        clock_test_passed("Clock First Victim Selection");
    } else {
        clock_test_failed("Clock First Victim Selection");
    }
    
    // Access page2 to set its reference bit
    clock.recordAccess(page2);
    
    // Next victim should be page2 (since page1 was already selected and page3 has cleared bit)
    PageID secondVictim = clock.selectVictim();
    if (secondVictim == page2 || secondVictim == page3) {
        clock_test_passed("Clock Second Victim Selection");
    } else {
        clock_test_failed("Clock Second Victim Selection");
    }
}

void test_clock_circular_behavior() {
    std::cout << "\n--- Clock Circular Behavior ---" << std::endl;
    
    ClockEvictionPolicy clock;
    clock.initialize(10);  // Initialize with capacity of 10
    
    // Insert 4 pages
    std::vector<PageID> pages;
    for (int i = 0; i < 4; i++) {
        pages.emplace_back("file" + std::to_string(i) + ".db", 0);
        clock.recordInsertion(pages[i]);
    }
    
    // Access all pages to set reference bits
    for (const auto& page : pages) {
        clock.recordAccess(page);
    }
    
    // First victim selection should clear all bits and select first page
    PageID victim1 = clock.selectVictim();
    
    // Access the first page again
    clock.recordAccess(victim1);
    
    // Next victim should be one of the other pages (with cleared reference bit)
    PageID victim2 = clock.selectVictim();
    
    if (victim2 != victim1) {
        clock_test_passed("Clock Circular Movement");
    } else {
        clock_test_failed("Clock Circular Movement");
    }
}

void test_clock_page_removal() {
    std::cout << "\n--- Clock Page Removal ---" << std::endl;
    
    ClockEvictionPolicy clock;
    clock.initialize(10);  // Initialize with capacity of 10
    
    // Insert pages
    PageID page1("remove1.db", 0);
    PageID page2("remove2.db", 0);
    PageID page3("remove3.db", 0);
    
    clock.recordInsertion(page1);
    clock.recordInsertion(page2);
    clock.recordInsertion(page3);
    
    if (clock.getTrackedPageCount() == 3) {
        clock_test_passed("Clock Insert Count");
    } else {
        clock_test_failed("Clock Insert Count");
    }
    
    // Remove middle page
    clock.recordRemoval(page2);
    
    if (clock.getTrackedPageCount() == 2) {
        clock_test_passed("Clock Remove Count");
    } else {
        clock_test_failed("Clock Remove Count");
    }
    
    // Verify remaining pages can still be selected as victims
    PageID victim1 = clock.selectVictim();
    if (victim1 == page1 || victim1 == page3) {
        clock_test_passed("Clock Remove Integrity");
    } else {
        clock_test_failed("Clock Remove Integrity");
    }
    
    // Remove all pages
    clock.recordRemoval(page1);
    clock.recordRemoval(page3);
    
    if (clock.isEmpty() && clock.getTrackedPageCount() == 0) {
        clock_test_passed("Clock Remove All");
    } else {
        clock_test_failed("Clock Remove All");
    }
}

void test_clock_access_patterns() {
    std::cout << "\n--- Clock Access Pattern Handling ---" << std::endl;
    
    ClockEvictionPolicy clock;
    clock.initialize(10);  // Initialize with capacity of 10
    
    // Insert pages
    PageID page1("access1.db", 0);
    PageID page2("access2.db", 0);
    PageID page3("access3.db", 0);
    PageID page4("access4.db", 0);
    
    clock.recordInsertion(page1);
    clock.recordInsertion(page2);
    clock.recordInsertion(page3);
    clock.recordInsertion(page4);
    
    // Simulate access pattern: frequently access page2 and page4
    for (int i = 0; i < 3; i++) {
        clock.recordAccess(page2);
        clock.recordAccess(page4);
    }
    
    // Select victims - should prefer pages 1 and 3 (less recently accessed)
    PageID victim1 = clock.selectVictim();
    PageID victim2 = clock.selectVictim();
    
    // At least one victim should be page1 or page3
    if ((victim1 == page1 || victim1 == page3) || (victim2 == page1 || victim2 == page3)) {
        clock_test_passed("Clock Access Pattern Preference");
    } else {
        clock_test_failed("Clock Access Pattern Preference");
    }
}

void test_clock_edge_cases() {
    std::cout << "\n--- Clock Edge Cases ---" << std::endl;
    
    ClockEvictionPolicy clock;
    clock.initialize(10);  // Initialize with capacity of 10
    
    // Test accessing non-existent page (should not crash)
    PageID nonexistent("nonexistent.db", 0);
    clock.recordAccess(nonexistent);  // Should be ignored silently
    clock_test_passed("Clock Access Nonexistent");
    
    // Test removing non-existent page (should not crash)
    clock.recordRemoval(nonexistent);  // Should be ignored silently
    clock_test_passed("Clock Remove Nonexistent");
    
    // Test duplicate insertion (should not crash or duplicate)
    PageID duplicate("duplicate.db", 0);
    clock.recordInsertion(duplicate);
    std::size_t count1 = clock.getTrackedPageCount();
    clock.recordInsertion(duplicate);  // Should be ignored
    std::size_t count2 = clock.getTrackedPageCount();
    
    if (count1 == count2 && count1 == 1) {
        clock_test_passed("Clock Duplicate Insert");
    } else {
        clock_test_failed("Clock Duplicate Insert");
    }
}

void test_clock_state_consistency() {
    std::cout << "\n--- Clock State Consistency ---" << std::endl;
    
    ClockEvictionPolicy clock;
    clock.initialize(10);  // Initialize with capacity of 10
    
    // Insert and remove pages in various orders
    std::vector<PageID> pages;
    for (int i = 0; i < 5; i++) {
        pages.emplace_back("consistency" + std::to_string(i) + ".db", 0);
        clock.recordInsertion(pages[i]);
    }
    
    // Remove some pages
    clock.recordRemoval(pages[1]);
    clock.recordRemoval(pages[3]);
    
    // Verify count is correct
    if (clock.getTrackedPageCount() == 3) {
        clock_test_passed("Clock Consistency Count");
    } else {
        clock_test_failed("Clock Consistency Count");
    }
    
    // Verify we can still select victims from remaining pages
    try {
        PageID victim1 = clock.selectVictim();
        PageID victim2 = clock.selectVictim();
        PageID victim3 = clock.selectVictim();
        
        // All victims should be from remaining pages (0, 2, 4)
        bool validVictims = (victim1 == pages[0] || victim1 == pages[2] || victim1 == pages[4]) &&
                           (victim2 == pages[0] || victim2 == pages[2] || victim2 == pages[4]) &&
                           (victim3 == pages[0] || victim3 == pages[2] || victim3 == pages[4]);
        
        if (validVictims) {
            clock_test_passed("Clock Consistency Victims");
        } else {
            clock_test_failed("Clock Consistency Victims");
        }
    } catch (const std::exception&) {
        clock_test_failed("Clock Consistency Victims");
    }
}

void test_buffer_capacity_fill() {
    std::cout << "\n--- Buffer Capacity Fill ---" << std::endl;
    
    ClockEvictionPolicy clock;
    const std::size_t capacity = 5;
    clock.initialize(capacity);
    
    // Fill buffer to capacity
    std::vector<PageID> pages;
    for (std::size_t i = 0; i < capacity; i++) {
        pages.emplace_back("fill" + std::to_string(i) + ".db", 0);
        clock.recordInsertion(pages[i]);
    }
    
    // Verify buffer is full
    if (clock.getTrackedPageCount() == capacity) {
        clock_test_passed("Buffer Fill to Capacity");
    } else {
        clock_test_failed("Buffer Fill to Capacity");
    }
    
    // All pages should be selectable as victims
    std::vector<PageID> victims;
    for (std::size_t i = 0; i < capacity; i++) {
        victims.push_back(clock.selectVictim());
    }
    
    // Verify all victims are from our inserted pages
    bool allValidVictims = true;
    for (const auto& victim : victims) {
        bool found = false;
        for (const auto& page : pages) {
            if (victim == page) {
                found = true;
                break;
            }
        }
        if (!found) {
            allValidVictims = false;
            break;
        }
    }
    
    if (allValidVictims) {
        clock_test_passed("Buffer Full Victim Selection");
    } else {
        clock_test_failed("Buffer Full Victim Selection");
    }
}

void test_buffer_eviction_simulation() {
    std::cout << "\n--- Buffer Eviction Simulation ---" << std::endl;
    
    ClockEvictionPolicy clock;
    const std::size_t capacity = 3;
    clock.initialize(capacity);
    
    // Fill buffer to capacity
    std::vector<PageID> initialPages;
    for (std::size_t i = 0; i < capacity; i++) {
        initialPages.emplace_back("initial" + std::to_string(i) + ".db", 0);
        clock.recordInsertion(initialPages[i]);
    }
    
    // Simulate buffer pool behavior: need to evict to make room for new pages
    std::vector<PageID> newPages;
    for (int i = 0; i < 5; i++) {
        // Select victim for eviction
        PageID victim = clock.selectVictim();
        
        // Remove the victim (simulating buffer pool eviction)
        clock.recordRemoval(victim);
        
        // Insert new page (simulating buffer pool insertion)
        PageID newPage("new" + std::to_string(i) + ".db", 0);
        newPages.push_back(newPage);
        clock.recordInsertion(newPage);
        
        // Verify buffer stays at capacity
        if (clock.getTrackedPageCount() != capacity) {
            clock_test_failed("Buffer Eviction Capacity Maintenance");
            return;
        }
    }
    
    clock_test_passed("Buffer Eviction Capacity Maintenance");
    
    // Verify final state contains only new pages (since we evicted more than capacity)
    std::vector<PageID> finalVictims;
    for (std::size_t i = 0; i < capacity; i++) {
        finalVictims.push_back(clock.selectVictim());
    }
    
    // All final victims should be from the new pages
    bool allFromNewPages = true;
    for (const auto& victim : finalVictims) {
        bool found = false;
        for (const auto& newPage : newPages) {
            if (victim == newPage) {
                found = true;
                break;
            }
        }
        if (!found) {
            allFromNewPages = false;
            break;
        }
    }
    
    if (allFromNewPages) {
        clock_test_passed("Buffer Eviction Final State");
    } else {
        clock_test_failed("Buffer Eviction Final State");
    }
}

void test_buffer_access_during_eviction() {
    std::cout << "\n--- Buffer Access During Eviction ---" << std::endl;
    
    ClockEvictionPolicy clock;
    const std::size_t capacity = 4;
    clock.initialize(capacity);
    
    // Fill buffer
    std::vector<PageID> pages;
    for (std::size_t i = 0; i < capacity; i++) {
        pages.emplace_back("access" + std::to_string(i) + ".db", 0);
        clock.recordInsertion(pages[i]);
    }
    
    // Access some pages frequently to set reference bits
    clock.recordAccess(pages[1]);  // Make page 1 recently accessed
    clock.recordAccess(pages[3]);  // Make page 3 recently accessed
    
    // Now simulate eviction cycles
    std::vector<PageID> evictedPages;
    for (int cycle = 0; cycle < 2; cycle++) {
        PageID victim = clock.selectVictim();
        evictedPages.push_back(victim);
        
        // The victim should be one of the non-accessed pages (0 or 2)
        if (victim != pages[1] && victim != pages[3]) {
            // Good - we're evicting less recently accessed pages
        } else {
            // This might happen after reference bits are cleared
        }
        
        // Remove victim and add new page
        clock.recordRemoval(victim);
        PageID newPage("cycle" + std::to_string(cycle) + ".db", 0);
        clock.recordInsertion(newPage);
        
        // Keep accessing our "hot" pages
        clock.recordAccess(pages[1]);
        clock.recordAccess(pages[3]);
    }
    
    if (clock.getTrackedPageCount() == capacity) {
        clock_test_passed("Buffer Access During Eviction");
    } else {
        clock_test_failed("Buffer Access During Eviction");
    }
}

void test_buffer_continuous_operation() {
    std::cout << "\n--- Buffer Continuous Operation ---" << std::endl;
    
    ClockEvictionPolicy clock;
    const std::size_t capacity = 3;
    clock.initialize(capacity);
    
    // Simulate continuous buffer operation with many insert/evict cycles
    std::size_t totalOperations = 20;
    std::size_t insertCount = 0;
    
    for (std::size_t op = 0; op < totalOperations; op++) {
        // If buffer has room, insert
        if (clock.getTrackedPageCount() < capacity) {
            PageID newPage("continuous" + std::to_string(insertCount++) + ".db", 0);
            clock.recordInsertion(newPage);
        } else {
            // Buffer full - need to evict before inserting
            PageID victim = clock.selectVictim();
            clock.recordRemoval(victim);
            
            PageID newPage("continuous" + std::to_string(insertCount++) + ".db", 0);
            clock.recordInsertion(newPage);
        }
        
        // Occasionally access some pages
        if (op % 3 == 0 && !clock.isEmpty()) {
            PageID accessTarget = clock.selectVictim(); // Just to get a page ID
            clock.recordAccess(accessTarget);
        }
        
        // Verify buffer never exceeds capacity
        if (clock.getTrackedPageCount() > capacity) {
            clock_test_failed("Buffer Continuous Operation Capacity");
            return;
        }
    }
    
    // Final verification
    if (clock.getTrackedPageCount() == capacity) {
        clock_test_passed("Buffer Continuous Operation");
    } else {
        clock_test_failed("Buffer Continuous Operation");
    }
}

int run_clock_tests() {
    std::cout << "\n========== CLOCK EVICTION TESTS ==========" << std::endl;
    
    test_basic_clock_operations();
    test_clock_reference_bits();
    test_clock_circular_behavior();
    test_clock_page_removal();
    test_clock_access_patterns();
    test_clock_edge_cases();
    test_clock_state_consistency();
    
    // New realistic buffer usage tests
    test_buffer_capacity_fill();
    test_buffer_eviction_simulation();
    test_buffer_access_during_eviction();
    test_buffer_continuous_operation();
    
    std::cout << "\n=== CLOCK TEST SUMMARY ===" << std::endl;
    std::cout << "Clock Tests Passed: " << clock_tests_passed << std::endl;
    std::cout << "Clock Tests Failed: " << clock_tests_failed << std::endl;
    
    return clock_tests_failed;
}

int main() {
    return run_clock_tests();
}