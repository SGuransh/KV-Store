#include "../LSM/LSMTree.hpp"
#include "../BTree/BTreeSST.hpp"
#include "../FileOperations.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <cstdio>

void cleanup_test_directory(const std::string& dir) {
    // Remove test directory and all files
    std::string cmd = "rm -rf " + dir;
    system(cmd.c_str());
}

void test_needs_compaction() {
    std::cout << "Testing needsCompaction method..." << std::endl;
    
    std::string testDir = "test_compaction_dir";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    // Create LSMTree
    LSMTree lsm(testDir);
    
    // Create two SST files with more data (BTree works better with larger datasets)
    BTreeSST builder;
    std::vector<std::pair<int, int>> data1;
    std::vector<std::pair<int, int>> data2;
    
    // Generate 20 pairs for each SST
    for (int i = 1; i <= 20; i++) {
        data1.push_back({i, i * 10});
    }
    for (int i = 21; i <= 40; i++) {
        data2.push_back({i, i * 10});
    }
    
    std::string sst1 = testDir + "/sst_1.txt";
    std::string sst2 = testDir + "/sst_2.txt";
    
    assert(builder.buildBTree(data1, sst1) == true);
    assert(builder.buildBTree(data2, sst2) == true);
    
    // Add first SST - should not trigger compaction
    assert(lsm.addSST("sst_1.txt", 0) == true);
    
    // Add second SST - should trigger compaction automatically
    assert(lsm.addSST("sst_2.txt", 0) == true);
    
    // After compaction, Level 0 should have 0 SSTs and Level 1 should have 1 SST
    lsm.printStructure();
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ needsCompaction test passed" << std::endl;
}

void test_compaction_cascade() {
    std::cout << "Testing compaction cascade..." << std::endl;
    
    std::string testDir = "test_cascade_dir";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Create 4 SST files with more data
    std::vector<std::pair<int, int>> data1, data2, data3, data4;
    
    for (int i = 1; i <= 10; i++) data1.push_back({i, i * 10});
    for (int i = 11; i <= 20; i++) data2.push_back({i, i * 10});
    for (int i = 21; i <= 30; i++) data3.push_back({i, i * 10});
    for (int i = 31; i <= 40; i++) data4.push_back({i, i * 10});
    
    assert(builder.buildBTree(data1, testDir + "/sst_1.txt") == true);
    assert(builder.buildBTree(data2, testDir + "/sst_2.txt") == true);
    assert(builder.buildBTree(data3, testDir + "/sst_3.txt") == true);
    assert(builder.buildBTree(data4, testDir + "/sst_4.txt") == true);
    
    // Add SSTs one by one
    // First two will compact to Level 1
    assert(lsm.addSST("sst_1.txt", 0) == true);
    assert(lsm.addSST("sst_2.txt", 0) == true);
    
    std::cout << "After first compaction:" << std::endl;
    lsm.printStructure();
    
    // Next two will compact to Level 1, then cascade to Level 2
    assert(lsm.addSST("sst_3.txt", 0) == true);
    assert(lsm.addSST("sst_4.txt", 0) == true);
    
    std::cout << "After cascade compaction:" << std::endl;
    lsm.printStructure();
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Compaction cascade test passed" << std::endl;
}

void test_get_operation() {
    std::cout << "Testing get operation..." << std::endl;
    
    std::string testDir = "test_get_dir";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Create SSTs at different levels with some overlapping keys
    std::vector<std::pair<int, int>> data1, data2, data3;
    
    // Data1: keys 1-10
    for (int i = 1; i <= 10; i++) {
        data1.push_back({i, i * 100});
    }
    
    // Data2: keys 11-20
    for (int i = 11; i <= 20; i++) {
        data2.push_back({i, i * 100});
    }
    
    // Data3: keys 5-15 (overlaps with both) - newer values
    for (int i = 5; i <= 15; i++) {
        data3.push_back({i, i * 999});
    }
    
    assert(builder.buildBTree(data1, testDir + "/sst_old1.txt") == true);
    assert(builder.buildBTree(data2, testDir + "/sst_old2.txt") == true);
    assert(builder.buildBTree(data3, testDir + "/sst_new.txt") == true);
    
    // Add to different levels (add to Level 2 first to avoid compaction)
    assert(lsm.addSST("sst_old1.txt", 2) == true);  // Level 2 (oldest)
    assert(lsm.addSST("sst_old2.txt", 2) == true);  // Level 2 (oldest)
    assert(lsm.addSST("sst_new.txt", 0) == true);  // Level 0 (newest)
    
    lsm.printStructure();
    
    // Test get operations
    int value;
    
    // Key exists only in Level 0
    assert(lsm.get(10, value) == true);
    assert(value == 10 * 999);  // Newer value from Level 0
    std::cout << "  ✓ Found key 10 with value " << value << " in Level 0" << std::endl;
    
    // Key exists only in Level 2
    assert(lsm.get(20, value) == true);
    assert(value == 20 * 100);
    std::cout << "  ✓ Found key 20 with value " << value << " in Level 2" << std::endl;
    
    // Key exists in both levels - should return Level 0 version (most recent)
    assert(lsm.get(5, value) == true);
    assert(value == 5 * 999);  // Should get newer value from Level 0, not 5*100 from Level 2
    std::cout << "  ✓ Found key 5 with value " << value << " (most recent from Level 0)" << std::endl;
    
    // Key doesn't exist
    assert(lsm.get(99, value) == false);
    std::cout << "  ✓ Key 99 not found (as expected)" << std::endl;
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Get operation test passed" << std::endl;
}

void test_scan_operation() {
    std::cout << "Testing scan operation with deduplication..." << std::endl;
    
    std::string testDir = "test_scan_dir";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Create SSTs with overlapping keys
    std::vector<std::pair<int, int>> data1, data2, data3;
    
    // Data1: keys 1-20 (odd numbers)
    for (int i = 1; i <= 20; i += 2) {
        data1.push_back({i, i * 100});
    }
    
    // Data2: keys 2-20 (even numbers)
    for (int i = 2; i <= 20; i += 2) {
        data2.push_back({i, i * 100});
    }
    
    // Data3: keys 5-15 (overlaps) - newer values
    for (int i = 5; i <= 15; i++) {
        data3.push_back({i, i * 999});
    }
    
    assert(builder.buildBTree(data1, testDir + "/sst_old1.txt") == true);
    assert(builder.buildBTree(data2, testDir + "/sst_old2.txt") == true);
    assert(builder.buildBTree(data3, testDir + "/sst_new.txt") == true);
    
    // Add to different levels (add to Level 2 first to avoid compaction)
    assert(lsm.addSST("sst_old1.txt", 2) == true);  // Level 2 (oldest)
    assert(lsm.addSST("sst_old2.txt", 2) == true);  // Level 2 (oldest)
    assert(lsm.addSST("sst_new.txt", 0) == true);  // Level 0 (newest)
    
    lsm.printStructure();
    
    // Test scan operation
    auto results = lsm.scan(5, 12);
    
    std::cout << "  Scan results for range [5, 12]:" << std::endl;
    for (const auto& pair : results) {
        std::cout << "    Key: " << pair.first << ", Value: " << pair.second << std::endl;
    }
    
    // Verify results - keys 5-12 should all be present
    assert(results.size() == 8);  // Keys 5, 6, 7, 8, 9, 10, 11, 12
    
    // Check that results are in sorted order and have correct values
    for (size_t i = 0; i < results.size(); i++) {
        int expectedKey = 5 + i;
        assert(results[i].first == expectedKey);
        
        // Keys 5-12 are all in data3 (Level 0), so should have value * 999
        assert(results[i].second == expectedKey * 999);
        std::cout << "    ✓ Key " << expectedKey << " has correct value " << results[i].second << std::endl;
    }
    
    std::cout << "  ✓ Scan returned correct deduplicated results in sorted order" << std::endl;
    
    // Test scan with no results
    auto emptyResults = lsm.scan(100, 200);
    assert(emptyResults.size() == 0);
    std::cout << "  ✓ Scan with no matching keys returned empty results" << std::endl;
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Scan operation test passed" << std::endl;
}

void test_update_handling() {
    std::cout << "Testing update handling (insert, update, verify)..." << std::endl;
    
    std::string testDir = "test_update_dir";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Insert initial data
    std::vector<std::pair<int, int>> data1;
    for (int i = 1; i <= 20; i++) {
        data1.push_back({i, i * 10});  // Initial values
    }
    
    assert(builder.buildBTree(data1, testDir + "/sst_1.txt") == true);
    assert(lsm.addSST("sst_1.txt", 0) == true);
    
    // Verify initial values
    int value;
    assert(lsm.get(5, value) == true);
    assert(value == 50);
    std::cout << "  ✓ Initial value for key 5: " << value << std::endl;
    
    // Insert updated data (simulating updates)
    std::vector<std::pair<int, int>> data2;
    for (int i = 5; i <= 15; i++) {
        data2.push_back({i, i * 100});  // Updated values
    }
    
    assert(builder.buildBTree(data2, testDir + "/sst_2.txt") == true);
    assert(lsm.addSST("sst_2.txt", 0) == true);
    
    // Verify updated values (should get newer values from Level 0)
    assert(lsm.get(5, value) == true);
    assert(value == 500);  // Updated value
    std::cout << "  ✓ Updated value for key 5: " << value << std::endl;
    
    assert(lsm.get(10, value) == true);
    assert(value == 1000);  // Updated value
    std::cout << "  ✓ Updated value for key 10: " << value << std::endl;
    
    // Verify non-updated keys still have original values
    assert(lsm.get(1, value) == true);
    assert(value == 10);  // Original value
    std::cout << "  ✓ Non-updated key 1 still has original value: " << value << std::endl;
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Update handling test passed" << std::endl;
}

void test_manifest_recovery() {
    std::cout << "Testing database close and reopen with manifest recovery..." << std::endl;
    
    std::string testDir = "test_recovery_dir";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    // Phase 1: Create database with some data
    {
        LSMTree lsm(testDir);
        BTreeSST builder;
        
        // Create and add multiple SSTs
        std::vector<std::pair<int, int>> data1, data2, data3;
        
        for (int i = 1; i <= 10; i++) data1.push_back({i, i * 10});
        for (int i = 11; i <= 20; i++) data2.push_back({i, i * 20});
        for (int i = 21; i <= 30; i++) data3.push_back({i, i * 30});
        
        assert(builder.buildBTree(data1, testDir + "/sst_a.txt") == true);
        assert(builder.buildBTree(data2, testDir + "/sst_b.txt") == true);
        assert(builder.buildBTree(data3, testDir + "/sst_c.txt") == true);
        
        assert(lsm.addSST("sst_a.txt", 0) == true);
        assert(lsm.addSST("sst_b.txt", 0) == true);  // This will trigger compaction
        assert(lsm.addSST("sst_c.txt", 0) == true);
        
        std::cout << "  Database structure before close:" << std::endl;
        lsm.printStructure();
        
        // Verify data is accessible
        int value;
        assert(lsm.get(5, value) == true);
        assert(value == 50);
        assert(lsm.get(15, value) == true);
        assert(value == 300);
        
        // LSMTree destructor will save manifest
    }
    
    // Phase 2: Reopen database and verify data is recovered
    {
        LSMTree lsm(testDir);
        
        std::cout << "  Database structure after reopen:" << std::endl;
        lsm.printStructure();
        
        // Verify all data is still accessible
        int value;
        
        assert(lsm.get(5, value) == true);
        assert(value == 50);
        std::cout << "  ✓ Key 5 recovered with value: " << value << std::endl;
        
        assert(lsm.get(15, value) == true);
        assert(value == 300);
        std::cout << "  ✓ Key 15 recovered with value: " << value << std::endl;
        
        assert(lsm.get(25, value) == true);
        assert(value == 750);
        std::cout << "  ✓ Key 25 recovered with value: " << value << std::endl;
        
        // Verify scan still works
        auto results = lsm.scan(10, 20);
        assert(results.size() == 11);  // Keys 10-20
        std::cout << "  ✓ Scan returned " << results.size() << " results after recovery" << std::endl;
    }
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Manifest recovery test passed" << std::endl;
}

int main() {
    std::cout << "Running Compaction and Integration tests..." << std::endl;
    std::cout << "================================" << std::endl;
    
    test_needs_compaction();
    test_compaction_cascade();
    test_get_operation();
    test_scan_operation();
    test_update_handling();
    test_manifest_recovery();
    
    std::cout << "================================" << std::endl;
    std::cout << "All Compaction and Integration tests passed! ✓" << std::endl;
    
    return 0;
}
