#include "../BTree/BTreeSST.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <unistd.h>

namespace {


// Helper function to manually create a test SST without using buildBTree
std::string createTestSST(const std::vector<std::pair<int, int>>& data) {
    char tmpTemplate[] = "/tmp/btree_test_XXXXXX";
    int fd = mkstemp(tmpTemplate);
    if (fd < 0) {
        std::cerr << "Failed to create temporary file" << std::endl;
        return "";
    }
    
    // Manually configure BuildContext similar to test_btree_internal_levels
    BuildContext ctx(tmpTemplate);
    ctx.fd = fd;
    ctx.leafNodeCount = (data.size() + MAX_LEAF_PAIRS - 1) / MAX_LEAF_PAIRS;
    
    // Set min/max keys
    ctx.minKey = data.empty() ? 0 : data.front().first;
    ctx.maxKey = data.empty() ? 0 : data.back().first;
    
    // Configure internal levels
    if (ctx.leafNodeCount > 1) {
        std::vector<size_t> levelSizes;
        size_t currentLevelSize = ctx.leafNodeCount;
        size_t totalInternalNodes = 0;

        while (currentLevelSize > 1) {
            currentLevelSize = (currentLevelSize + MAX_INTERNAL_CHILDREN - 1) / MAX_INTERNAL_CHILDREN;
            levelSizes.push_back(currentLevelSize);
            totalInternalNodes += currentLevelSize;
        }

        ctx.internalLevelCount = levelSizes.size();
        ctx.totalInternalNodes = totalInternalNodes;
        ctx.treeHeight = ctx.internalLevelCount + 1;

        if (ctx.internalLevelCount > 0) {
            ctx.internalLevelSizes = new size_t[ctx.internalLevelCount];
            for (size_t i = 0; i < ctx.internalLevelCount; ++i) {
                ctx.internalLevelSizes[i] = levelSizes[i];
            }
        }
    } else {
        ctx.internalLevelCount = 0;
        ctx.totalInternalNodes = 0;
        ctx.treeHeight = 1;
    }
    
    // Write leaf pages manually
    BTreeSST sst;
    int32_t* max_per_node = sst.buildLeafNodes(ctx, data);
    
    // Build internal levels if needed
    if (ctx.leafNodeCount > 1) {
        sst.buildInternalLevels(ctx, max_per_node);
    }
    
    delete[] max_per_node;
    
    return std::string(tmpTemplate);
}

// Test 1: Single leaf tree (height = 1)
bool test_single_leaf() {
    std::cout << "\n[Test 1: Single Leaf Tree]" << std::endl;
    
    std::vector<std::pair<int, int>> data = {{1, 10}, {2, 20}, {3, 30}};
    std::string filename = createTestSST(data);
    if (filename.empty()) return false;
    
    BTreeSST sst;
    
    // Manually create metadata since we can't serialize it with small PAGE_SIZE
    MetadataPage metadata;
    metadata.minKey = data.front().first;
    metadata.maxKey = data.back().first;
    metadata.treeHeight = 1; // Single leaf for 3 items with MAX_LEAF_PAIRS = 1
    metadata.leafCount = (data.size() + MAX_LEAF_PAIRS - 1) / MAX_LEAF_PAIRS;
    metadata.lastLeafPairs = data.size() % MAX_LEAF_PAIRS;
    if (metadata.lastLeafPairs == 0) metadata.lastLeafPairs = MAX_LEAF_PAIRS;
    metadata.rootPageId = 1; // Single leaf becomes root
    
    bool passed = true;
    
    // Test existing keys
    for (const auto& pair : data) {
        int value;
        if (!sst.getBTreeSearch(pair.first, value, filename, metadata)) {
            std::cerr << "  Failed: key " << pair.first << " should be found" << std::endl;
            passed = false;
        } else if (value != pair.second) {
            std::cerr << "  Failed: key " << pair.first << " should return " 
                      << pair.second << ", got " << value << std::endl;
            passed = false;
        }
    }
    
    // Test non-existing keys
    int value;
    if (sst.getBTreeSearch(0, value, filename, metadata)) {
        std::cerr << "  Failed: key 0 should not be found" << std::endl;
        passed = false;
    }
    if (sst.getBTreeSearch(5, value, filename, metadata)) {
        std::cerr << "  Failed: key 5 should not be found" << std::endl;
        passed = false;
    }
    
    unlink(filename.c_str());
    
    if (passed) {
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  FAILED" << std::endl;
    }
    
    return passed;
}

// Test 2: Multi-level tree with multiple leaves
bool test_multi_level_tree() {
    std::cout << "\n[Test 2: Multi-Level Tree]" << std::endl;
    
    // Create data that will require multiple leaf nodes
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 20; i++) {
        data.push_back({i, i * 10});
    }
    
    std::string filename = createTestSST(data);
    if (filename.empty()) return false;
    
    BTreeSST sst;
    
    // Manually create metadata for multi-level tree
    MetadataPage metadata;
    metadata.minKey = data.front().first;
    metadata.maxKey = data.back().first;
    metadata.leafCount = (data.size() + MAX_LEAF_PAIRS - 1) / MAX_LEAF_PAIRS;
    metadata.lastLeafPairs = data.size() % MAX_LEAF_PAIRS;
    if (metadata.lastLeafPairs == 0) metadata.lastLeafPairs = MAX_LEAF_PAIRS;
    
    // Calculate tree height and internal structure
    size_t leafCount = metadata.leafCount;
    if (leafCount == 1) {
        metadata.treeHeight = 1;
        metadata.rootPageId = 1; // Single leaf
    } else {
        // Calculate internal levels
        size_t currentLevelSize = leafCount;
        size_t totalInternalNodes = 0;
        size_t internalLevelCount = 0;
        
        while (currentLevelSize > 1) {
            currentLevelSize = (currentLevelSize + MAX_INTERNAL_CHILDREN - 1) / MAX_INTERNAL_CHILDREN;
            totalInternalNodes += currentLevelSize;
            internalLevelCount++;
        }
        
        metadata.treeHeight = internalLevelCount + 1;
        metadata.rootPageId = 1; // First internal node
        
        // Set per-level metadata (simplified - assumes full nodes except last)
        metadata.nodesPerLevel[0] = leafCount; // Level 0 = leaves
        metadata.lastNodeKeys[0] = metadata.lastLeafPairs;
    }
    
    std::cout << "  Tree height: " << metadata.treeHeight << std::endl;
    std::cout << "  Leaf count: " << metadata.leafCount << std::endl;
    
    bool passed = true;
    
    // Test all existing keys
    for (const auto& pair : data) {
        int value;
        if (!sst.getBTreeSearch(pair.first, value, filename, metadata)) {
            std::cerr << "  Failed: key " << pair.first << " should be found" << std::endl;
            passed = false;
        } else if (value != pair.second) {
            std::cerr << "  Failed: key " << pair.first << " should return " 
                      << pair.second << ", got " << value << std::endl;
            passed = false;
        }
    }
    
    // Test keys outside range
    int value;
    if (sst.getBTreeSearch(0, value, filename, metadata)) {
        std::cerr << "  Failed: key 0 should not be found (below range)" << std::endl;
        passed = false;
    }
    if (sst.getBTreeSearch(25, value, filename, metadata)) {
        std::cerr << "  Failed: key 25 should not be found (above range)" << std::endl;
        passed = false;
    }
    
    // Test keys within range but not present
    if (data.size() > 2) {
        // Test a key between existing keys (e.g., between key 5 and 6, test 5.5 -> use key 50)
        if (sst.getBTreeSearch(50, value, filename, metadata)) {
            std::cerr << "  Failed: key 50 should not be found (not in data)" << std::endl;
            passed = false;
        }
    }
    
    unlink(filename.c_str());
    
    if (passed) {
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  FAILED" << std::endl;
    }
    
    return passed;
}

// Test 3: Large dataset to test deeper tree
bool test_large_dataset() {
    std::cout << "\n[Test 3: Large Dataset]" << std::endl;
    
    // Create a larger dataset that will create a deeper tree
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i * 2, i * 20}); // Even keys: 2, 4, 6, ..., 200
    }
    
    std::string filename = createTestSST(data);
    if (filename.empty()) return false;
    
    BTreeSST sst;
    
    // Manually create metadata for large dataset
    MetadataPage metadata;
    metadata.minKey = data.front().first;
    metadata.maxKey = data.back().first;
    metadata.leafCount = (data.size() + MAX_LEAF_PAIRS - 1) / MAX_LEAF_PAIRS;
    metadata.lastLeafPairs = data.size() % MAX_LEAF_PAIRS;
    if (metadata.lastLeafPairs == 0) metadata.lastLeafPairs = MAX_LEAF_PAIRS;
    
    // Calculate tree height for large dataset
    size_t leafCount = metadata.leafCount;
    if (leafCount == 1) {
        metadata.treeHeight = 1;
        metadata.rootPageId = 1;
    } else {
        size_t currentLevelSize = leafCount;
        size_t totalInternalNodes = 0;
        size_t internalLevelCount = 0;
        
        while (currentLevelSize > 1) {
            currentLevelSize = (currentLevelSize + MAX_INTERNAL_CHILDREN - 1) / MAX_INTERNAL_CHILDREN;
            totalInternalNodes += currentLevelSize;
            internalLevelCount++;
        }
        
        metadata.treeHeight = internalLevelCount + 1;
        metadata.rootPageId = 1;
        
        metadata.nodesPerLevel[0] = leafCount;
        metadata.lastNodeKeys[0] = metadata.lastLeafPairs;
    }
    
    std::cout << "  Tree height: " << metadata.treeHeight << std::endl;
    std::cout << "  Leaf count: " << metadata.leafCount << std::endl;
    std::cout << "  Key range: " << metadata.minKey << " to " << metadata.maxKey << std::endl;
    
    bool passed = true;
    
    // Test some existing keys (not all to keep test fast)
    std::vector<int> testKeys = {2, 10, 50, 100, 150, 200};
    for (int key : testKeys) {
        int expectedValue = key * 10; // Since we stored i*2 -> i*20, so key*10
        int value;
        if (!sst.getBTreeSearch(key, value, filename, metadata)) {
            std::cerr << "  Failed: key " << key << " should be found" << std::endl;
            passed = false;
        } else if (value != expectedValue) {
            std::cerr << "  Failed: key " << key << " should return " 
                      << expectedValue << ", got " << value << std::endl;
            passed = false;
        }
    }
    
    // Test non-existing keys (odd numbers in our range)
    std::vector<int> nonExistentKeys = {1, 3, 51, 99, 201};
    for (int key : nonExistentKeys) {
        int value;
        if (sst.getBTreeSearch(key, value, filename, metadata)) {
            std::cerr << "  Failed: key " << key << " should not be found" << std::endl;
            passed = false;
        }
    }
    
    unlink(filename.c_str());
    
    if (passed) {
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  FAILED" << std::endl;
    }
    
    return passed;
}

// Test 4: Edge cases
bool test_edge_cases() {
    std::cout << "\n[Test 4: Edge Cases]" << std::endl;
    
    bool passed = true;
    
    // Test with single key-value pair
    {
        std::vector<std::pair<int, int>> data = {{42, 420}};
        std::string filename = createTestSST(data);
        if (filename.empty()) return false;
        
        BTreeSST sst;
        
        // Manually create metadata for single key test
        MetadataPage metadata;
        metadata.minKey = data.front().first;
        metadata.maxKey = data.back().first;
        metadata.treeHeight = 1;
        metadata.leafCount = 1;
        metadata.lastLeafPairs = 1;
        metadata.rootPageId = 1;
        
        int value;
        if (!sst.getBTreeSearch(42, value, filename, metadata) || value != 420) {
            std::cerr << "  Failed: single key test" << std::endl;
            passed = false;
        }
        
        if (sst.getBTreeSearch(41, value, filename, metadata) || 
            sst.getBTreeSearch(43, value, filename, metadata)) {
            std::cerr << "  Failed: single key test - non-existent keys found" << std::endl;
            passed = false;
        }
        
        unlink(filename.c_str());
    }
    
    // Test with keys at boundaries
    {
        std::vector<std::pair<int, int>> data = {{-100, -1000}, {0, 0}, {100, 1000}};
        std::string filename = createTestSST(data);
        if (filename.empty()) return false;
        
        BTreeSST sst;
        
        // Manually create metadata for boundary test
        MetadataPage metadata;
        metadata.minKey = data.front().first;
        metadata.maxKey = data.back().first;
        metadata.leafCount = (data.size() + MAX_LEAF_PAIRS - 1) / MAX_LEAF_PAIRS;
        metadata.lastLeafPairs = data.size() % MAX_LEAF_PAIRS;
        if (metadata.lastLeafPairs == 0) metadata.lastLeafPairs = MAX_LEAF_PAIRS;
        metadata.treeHeight = (metadata.leafCount == 1) ? 1 : 2; // Simple approximation
        metadata.rootPageId = 1;
        
        int value;
        // Test all boundary values
        if (!sst.getBTreeSearch(-100, value, filename, metadata) || value != -1000 ||
            !sst.getBTreeSearch(0, value, filename, metadata) || value != 0 ||
            !sst.getBTreeSearch(100, value, filename, metadata) || value != 1000) {
            std::cerr << "  Failed: boundary values test" << std::endl;
            passed = false;
        }
        
        // Test values outside boundaries
        if (sst.getBTreeSearch(-101, value, filename, metadata) ||
            sst.getBTreeSearch(101, value, filename, metadata)) {
            std::cerr << "  Failed: values outside boundaries should not be found" << std::endl;
            passed = false;
        }
        
        unlink(filename.c_str());
    }
    
    if (passed) {
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  FAILED" << std::endl;
    }
    
    return passed;
}

// Test 5: Very large dataset (256,000 entries) to test deep tree and performance
bool test_very_large_dataset() {
    std::cout << "\n[Test 5: Very Large Dataset (256,000 entries)]" << std::endl;
    
    // Create a very large dataset that will create a deep tree
    std::vector<std::pair<int, int>> data;
    data.reserve(256000);
    for (int i = 1; i <= 256000; i++) {
        data.push_back({i, i * 10}); // Keys: 1, 2, 3, ..., 256000
    }
    
    std::cout << "  Creating SST with " << data.size() << " entries..." << std::endl;
    std::string filename = createTestSST(data);
    if (filename.empty()) return false;
    
    BTreeSST sst;
    
    // Manually create metadata for very large dataset
    MetadataPage metadata;
    metadata.minKey = data.front().first;
    metadata.maxKey = data.back().first;
    metadata.leafCount = (data.size() + MAX_LEAF_PAIRS - 1) / MAX_LEAF_PAIRS;
    metadata.lastLeafPairs = data.size() % MAX_LEAF_PAIRS;
    if (metadata.lastLeafPairs == 0) metadata.lastLeafPairs = MAX_LEAF_PAIRS;
    
    // Calculate tree height for very large dataset
    size_t leafCount = metadata.leafCount;
    if (leafCount == 1) {
        metadata.treeHeight = 1;
        metadata.rootPageId = 1;
    } else {
        size_t currentLevelSize = leafCount;
        size_t totalInternalNodes = 0;
        size_t internalLevelCount = 0;
        
        while (currentLevelSize > 1) {
            currentLevelSize = (currentLevelSize + MAX_INTERNAL_CHILDREN - 1) / MAX_INTERNAL_CHILDREN;
            totalInternalNodes += currentLevelSize;
            internalLevelCount++;
        }
        
        metadata.treeHeight = internalLevelCount + 1;
        metadata.rootPageId = 1;
        
        metadata.nodesPerLevel[0] = leafCount;
        metadata.lastNodeKeys[0] = metadata.lastLeafPairs;
    }
    
    std::cout << "  Tree height: " << metadata.treeHeight << std::endl;
    std::cout << "  Leaf count: " << metadata.leafCount << std::endl;
    std::cout << "  Key range: " << metadata.minKey << " to " << metadata.maxKey << std::endl;
    std::cout << "  MAX_LEAF_PAIRS: " << MAX_LEAF_PAIRS << std::endl;
    std::cout << "  MAX_INTERNAL_CHILDREN: " << MAX_INTERNAL_CHILDREN << std::endl;
    
    bool passed = true;
    
    // Test keys at various positions in the dataset
    std::vector<int> testKeys = {
        1,          // First key
        100,        // Early position
        1000,       // Mid-early position
        50000,      // Quarter point
        128000,     // Midpoint
        200000,     // Three-quarter point
        255000,     // Near end
        256000      // Last key
    };
    
    std::cout << "  Testing " << testKeys.size() << " key lookups..." << std::endl;
    for (int key : testKeys) {
        int expectedValue = key * 10;
        int value;
        if (!sst.getBTreeSearch(key, value, filename, metadata)) {
            std::cerr << "  Failed: key " << key << " should be found" << std::endl;
            passed = false;
        } else if (value != expectedValue) {
            std::cerr << "  Failed: key " << key << " should return " 
                      << expectedValue << ", got " << value << std::endl;
            passed = false;
        }
    }
    
    // Test non-existing keys at various positions
    std::vector<int> nonExistentKeys = {0, -1, 256001, 300000, 500000};
    std::cout << "  Testing " << nonExistentKeys.size() << " non-existent key lookups..." << std::endl;
    for (int key : nonExistentKeys) {
        int value;
        if (sst.getBTreeSearch(key, value, filename, metadata)) {
            std::cerr << "  Failed: key " << key << " should not be found" << std::endl;
            passed = false;
        }
    }
    
    // Test a random sample of keys for thorough validation
    std::cout << "  Testing random sample of 100 keys..." << std::endl;
    for (int i = 0; i < 100; i++) {
        int key = (i * 2560) + 1; // Sample keys: 1, 2561, 5121, ...
        if (key > 256000) break;
        
        int expectedValue = key * 10;
        int value;
        if (!sst.getBTreeSearch(key, value, filename, metadata)) {
            std::cerr << "  Failed: random key " << key << " should be found" << std::endl;
            passed = false;
        } else if (value != expectedValue) {
            std::cerr << "  Failed: random key " << key << " should return " 
                      << expectedValue << ", got " << value << std::endl;
            passed = false;
        }
    }
    
    unlink(filename.c_str());
    
    if (passed) {
        std::cout << "  PASSED" << std::endl;
    } else {
        std::cout << "  FAILED" << std::endl;
    }
    
    return passed;
}

} // namespace

int main() {
    std::cout << "╔═══════════════════════════════════════╗" << std::endl;
    std::cout << "║  B-Tree getBTreeSearch() Tests        ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════╝" << std::endl;
    
    bool allPassed = true;
    
    allPassed &= test_single_leaf();
    allPassed &= test_multi_level_tree();
    allPassed &= test_large_dataset();
    allPassed &= test_edge_cases();
    allPassed &= test_very_large_dataset();
    
    std::cout << "\n╔═══════════════════════════════════════╗" << std::endl;
    if (allPassed) {
        std::cout << "║  All getBTreeSearch() tests PASSED!   ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════╝" << std::endl;
        return 0;
    } else {
        std::cout << "║  Some getBTreeSearch() tests FAILED!  ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════╝" << std::endl;
        return 1;
    }
}
