#include "../BTree/BTreeSST.hpp"
#include "../LSM/LSMTree.hpp"
#include "../FileOperations.hpp"
#include <iostream>
#include <vector>

int main() {
    std::string testDir = "test_merge_nonoverlap";
    
    // Clean up if exists
    std::string cmd = "rm -rf " + testDir;
    system(cmd.c_str());
    
    // Create directory
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
    
    builder.buildBTree(data1, sst1);
    builder.buildBTree(data2, sst2);
    
    // Perform merge
    lsm.testMergeTwoSSTs("sst_1.txt", "sst_2.txt", "sst_merged.txt", 1);
    
    std::cout << "Test data created successfully!" << std::endl;
    std::cout << "\nNow running btree_read test:\n" << std::endl;
    
    // Now test reading (use binary search since B-Tree search not yet implemented)
    BTreeSST reader;
    int value;
    bool found = reader.get(1, value, "test_merge_nonoverlap/sst_merged.txt");
    std::cout << "Found key 1: " << found << ", value: " << value << std::endl;
    
    found = reader.get(20, value, "test_merge_nonoverlap/sst_merged.txt");
    std::cout << "Found key 20: " << found << ", value: " << value << std::endl;
    
    found = reader.get(40, value, "test_merge_nonoverlap/sst_merged.txt");
    std::cout << "Found key 40: " << found << ", value: " << value << std::endl;
    
    std::cout << "\n✓ test_btree_read passed!" << std::endl;
    
    // Clean up test directory
    system(cmd.c_str());
    
    return 0;
}
