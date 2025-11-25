#include "../LSM/LSMTree.hpp"
#include "../BTree/BTreeSST.hpp"
#include "../FileOperations.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <cstdio>
#include <algorithm>

void cleanup_test_directory(const std::string& dir) {
    std::string cmd = "rm -rf " + dir;
    system(cmd.c_str());
}

void test_merge_non_overlapping_keys() {
    std::cout << "Testing merge with non-overlapping keys..." << std::endl;
    
    std::string testDir = "test_merge_nonoverlap";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Create two SSTs with non-overlapping keys
    std::vector<std::pair<int, int>> data1, data2;
    
    // SST1: keys 1-20
    for (int i = 1; i <= 20; i++) {
        data1.push_back({i, i * 10});
    }
    
    // SST2: keys 21-40
    for (int i = 21; i <= 40; i++) {
        data2.push_back({i, i * 10});
    }
    
    std::string sst1 = testDir + "/sst_1.txt";
    std::string sst2 = testDir + "/sst_2.txt";
    std::string output = testDir + "/sst_merged.txt";
    
    assert(builder.buildBTree(data1, sst1) == true);
    assert(builder.buildBTree(data2, sst2) == true);
    
    // Perform merge
    assert(lsm.testMergeTwoSSTs("sst_1.txt", "sst_2.txt", "sst_merged.txt", 1) == true);
    
    // Verify merged SST contains all keys
    BTreeSST reader;
    std::string fullOutput = testDir + "/sst_merged.txt";
    for (int i = 1; i <= 40; i++) {
        int value;
        assert(reader.get(i, value, fullOutput) == true);  // Use B-Tree search
        assert(value == i * 10);
    }
    
    std::cout << "  ✓ All 40 keys present in merged SST" << std::endl;
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Non-overlapping keys merge test passed" << std::endl;
}

void test_merge_overlapping_keys() {
    std::cout << "Testing merge with overlapping keys..." << std::endl;
    
    std::string testDir = "test_merge_overlap";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Create two SSTs with overlapping keys
    std::vector<std::pair<int, int>> data1, data2;
    
    // SST1: keys 1-30
    for (int i = 1; i <= 30; i++) {
        data1.push_back({i, i * 10});
    }
    
    // SST2: keys 20-50 (overlaps with SST1 on keys 20-30)
    for (int i = 20; i <= 50; i++) {
        data2.push_back({i, i * 10});
    }
    
    std::string sst1 = testDir + "/sst_1.txt";
    std::string sst2 = testDir + "/sst_2.txt";
    std::string output = testDir + "/sst_merged.txt";
    
    assert(builder.buildBTree(data1, sst1) == true);
    assert(builder.buildBTree(data2, sst2) == true);
    
    // Perform merge
    assert(lsm.testMergeTwoSSTs("sst_1.txt", "sst_2.txt", "sst_merged.txt", 1) == true);
    
    // Verify merged SST contains all unique keys (1-50)
    BTreeSST reader;
    std::string fullOutput = testDir + "/sst_merged.txt";
    for (int i = 1; i <= 50; i++) {
        int value;
        assert(reader.get(i, value, fullOutput) == true);  // Use B-Tree search
        assert(value == i * 10);
    }
    
    std::cout << "  ✓ All 50 unique keys present in merged SST" << std::endl;
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Overlapping keys merge test passed" << std::endl;
}

void test_merge_duplicate_key_resolution() {
    std::cout << "Testing duplicate key resolution (lower level wins)..." << std::endl;
    
    std::string testDir = "test_merge_duplicates";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Create two SSTs with same keys but different values
    std::vector<std::pair<int, int>> data1, data2;
    
    // SST1 (lower level, newer): keys 1-20 with value = key * 100
    for (int i = 1; i <= 20; i++) {
        data1.push_back({i, i * 100});
    }
    
    // SST2 (same level, will be second parameter): keys 1-20 with value = key * 999
    for (int i = 1; i <= 20; i++) {
        data2.push_back({i, i * 999});
    }
    
    std::string sst1 = testDir + "/sst_1.txt";
    std::string sst2 = testDir + "/sst_2.txt";
    std::string output = testDir + "/sst_merged.txt";
    
    assert(builder.buildBTree(data1, sst1) == true);
    assert(builder.buildBTree(data2, sst2) == true);
    
    // Perform merge - sst1 is first parameter (lower level, should win)
    assert(lsm.testMergeTwoSSTs("sst_1.txt", "sst_2.txt", "sst_merged.txt", 1) == true);
    
    // Verify merged SST contains values from SST1 (first parameter)
    BTreeSST reader;
    std::string fullOutput = testDir + "/sst_merged.txt";
    for (int i = 1; i <= 20; i++) {
        int value;
        assert(reader.get(i, value, fullOutput) == true);  // Use B-Tree search
        // Should have value from sst1 (i * 100), not sst2 (i * 999)
        assert(value == i * 100);
    }
    
    std::cout << "  ✓ Duplicate keys resolved correctly (first SST wins)" << std::endl;
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Duplicate key resolution test passed" << std::endl;
}

void test_merge_various_sizes() {
    std::cout << "Testing merge with various data sizes..." << std::endl;
    
    std::string testDir = "test_merge_sizes";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Test 1: Small datasets (10 pairs each)
    {
        std::vector<std::pair<int, int>> data1, data2;
        for (int i = 1; i <= 10; i++) data1.push_back({i, i});
        for (int i = 11; i <= 20; i++) data2.push_back({i, i});
        
        assert(builder.buildBTree(data1, testDir + "/small_1.txt") == true);
        assert(builder.buildBTree(data2, testDir + "/small_2.txt") == true);
        assert(lsm.testMergeTwoSSTs("small_1.txt", "small_2.txt", "small_merged.txt", 1) == true);
        
        BTreeSST reader;
        int value;
        assert(reader.get(1, value, testDir + "/small_merged.txt") == true);
        assert(reader.get(20, value, testDir + "/small_merged.txt") == true);
        std::cout << "  ✓ Small dataset merge (20 pairs total)" << std::endl;
    }
    
    // Test 2: Medium datasets (100 pairs each)
    {
        std::vector<std::pair<int, int>> data1, data2;
        for (int i = 1; i <= 100; i++) data1.push_back({i, i});
        for (int i = 101; i <= 200; i++) data2.push_back({i, i});
        
        assert(builder.buildBTree(data1, testDir + "/medium_1.txt") == true);
        assert(builder.buildBTree(data2, testDir + "/medium_2.txt") == true);
        assert(lsm.testMergeTwoSSTs("medium_1.txt", "medium_2.txt", "medium_merged.txt", 1) == true);
        
        BTreeSST reader;
        int value;
        assert(reader.get(1, value, testDir + "/medium_merged.txt") == true);
        assert(reader.get(200, value, testDir + "/medium_merged.txt") == true);
        std::cout << "  ✓ Medium dataset merge (200 pairs total)" << std::endl;
    }
    
    // Test 3: Large datasets (500 pairs each)
    {
        std::vector<std::pair<int, int>> data1, data2;
        for (int i = 1; i <= 500; i++) data1.push_back({i, i});
        for (int i = 501; i <= 1000; i++) data2.push_back({i, i});
        
        assert(builder.buildBTree(data1, testDir + "/large_1.txt") == true);
        assert(builder.buildBTree(data2, testDir + "/large_2.txt") == true);
        assert(lsm.testMergeTwoSSTs("large_1.txt", "large_2.txt", "large_merged.txt", 1) == true);
        
        BTreeSST reader;
        int value;
        assert(reader.get(1, value, testDir + "/large_merged.txt") == true);
        assert(reader.get(1000, value, testDir + "/large_merged.txt") == true);
        std::cout << "  ✓ Large dataset merge (1000 pairs total)" << std::endl;
    }
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Various sizes merge test passed" << std::endl;
}

void test_merge_interleaved_keys() {
    std::cout << "Testing merge with interleaved keys..." << std::endl;
    
    std::string testDir = "test_merge_interleaved";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    LSMTree lsm(testDir);
    BTreeSST builder;
    
    // Create two SSTs with interleaved keys
    std::vector<std::pair<int, int>> data1, data2;
    
    // SST1: odd keys 1, 3, 5, ..., 39
    for (int i = 1; i <= 40; i += 2) {
        data1.push_back({i, i * 10});
    }
    
    // SST2: even keys 2, 4, 6, ..., 40
    for (int i = 2; i <= 40; i += 2) {
        data2.push_back({i, i * 10});
    }
    
    std::string sst1 = testDir + "/sst_1.txt";
    std::string sst2 = testDir + "/sst_2.txt";
    std::string output = testDir + "/sst_merged.txt";
    
    assert(builder.buildBTree(data1, sst1) == true);
    assert(builder.buildBTree(data2, sst2) == true);
    
    // Perform merge
    assert(lsm.testMergeTwoSSTs("sst_1.txt", "sst_2.txt", "sst_merged.txt", 1) == true);
    
    // Verify all keys 1-40 are present in sorted order
    BTreeSST reader;
    std::string fullOutput = testDir + "/sst_merged.txt";
    for (int i = 1; i <= 40; i++) {
        int value;
        assert(reader.get(i, value, fullOutput) == true);  // Use B-Tree search
        assert(value == i * 10);
    }
    
    std::cout << "  ✓ All 40 interleaved keys merged correctly" << std::endl;
    
    cleanup_test_directory(testDir);
    
    std::cout << "✓ Interleaved keys merge test passed" << std::endl;
}

int main() {
    std::cout << "Running Merge Algorithm tests..." << std::endl;
    std::cout << "================================" << std::endl;
    
    test_merge_non_overlapping_keys();
    test_merge_overlapping_keys();
    test_merge_duplicate_key_resolution();
    test_merge_various_sizes();
    test_merge_interleaved_keys();
    
    std::cout << "================================" << std::endl;
    std::cout << "All Merge Algorithm tests passed! ✓" << std::endl;
    
    return 0;
}
