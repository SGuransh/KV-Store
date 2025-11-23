#include "../BTree/BTreeSST.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <unistd.h>

namespace {

// Helper function to manually create a test SST without using buildBTree
std::string createTestSST(const std::vector<std::pair<int, int>>& data) {
    char tmpTemplate[] = "/tmp/btree_scan_test_XXXXXX";
    int fd = mkstemp(tmpTemplate);
    if (fd < 0) {
        std::cerr << "Failed to create temporary file" << std::endl;
        return "";
    }
    
    // Manually configure BuildContext similar to test_btree_get
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
        
        // Allocate and populate internalLevelSizes array
        ctx.internalLevelSizes = new size_t[ctx.internalLevelCount];
        for (size_t i = 0; i < ctx.internalLevelCount; i++) {
            ctx.internalLevelSizes[i] = levelSizes[i];
        }
    } else {
        ctx.internalLevelCount = 0;
        ctx.totalInternalNodes = 0;
        ctx.treeHeight = 1;
    }
    
    // Build the tree using existing functions
    BTreeSST sst;
    int32_t* max_per_node = sst.buildLeafNodes(ctx, data);
    
    if (ctx.leafNodeCount > 1) {
        sst.buildInternalLevels(ctx, max_per_node);
    }
    
    delete[] max_per_node;
    
    // Manually create metadata
    MetadataPage metadata;
    metadata.minKey = ctx.minKey;
    metadata.maxKey = ctx.maxKey;
    metadata.leafCount = ctx.leafNodeCount;
    metadata.lastLeafPairs = data.size() % MAX_LEAF_PAIRS;
    if (metadata.lastLeafPairs == 0 && !data.empty()) {
        metadata.lastLeafPairs = MAX_LEAF_PAIRS;
    }
    metadata.treeHeight = ctx.treeHeight;
    metadata.rootPageId = (ctx.treeHeight == 1) ? 1 : 1;
    
    // Write metadata to page 0
    Page metadataPage;
    metadata.serialize(metadataPage);
    lseek(ctx.fd, 0, SEEK_SET);
    write(ctx.fd, metadataPage.getData(), Page::PAGE_SIZE);
    
    close(ctx.fd);
    ctx.fd = -1;
    
    return tmpTemplate;
}

void testSingleLeafScan() {
    std::cout << "\n[Test 1: Single Leaf Scan]" << std::endl;
    
    // Create test data
    std::vector<std::pair<int, int>> data = {
        {10, 100}, {20, 200}, {30, 300}, {40, 400}, {50, 500}
    };
    
    std::string filename = createTestSST(data);
    if (filename.empty()) {
        std::cerr << "  FAILED: Could not create test file" << std::endl;
        return;
    }
    
    BTreeSST sst;
    
    // Test 1: Full range scan
    auto result1 = sst.scan(10, 50, filename, true);
    assert(result1.size() == 5);
    assert(result1[0].first == 10 && result1[0].second == 100);
    assert(result1[4].first == 50 && result1[4].second == 500);
    std::cout << "  Full range scan [10,50]: " << result1.size() << " pairs - PASSED" << std::endl;
    
    // Test 2: Partial range scan
    auto result2 = sst.scan(20, 40, filename, true);
    assert(result2.size() == 3);
    assert(result2[0].first == 20 && result2[0].second == 200);
    assert(result2[2].first == 40 && result2[2].second == 400);
    std::cout << "  Partial range scan [20,40]: " << result2.size() << " pairs - PASSED" << std::endl;
    
    // Test 3: Single key range
    auto result3 = sst.scan(30, 30, filename, true);
    assert(result3.size() == 1);
    assert(result3[0].first == 30 && result3[0].second == 300);
    std::cout << "  Single key scan [30,30]: " << result3.size() << " pair - PASSED" << std::endl;
    
    // Test 4: Empty range (no keys in range)
    auto result4 = sst.scan(35, 38, filename, true);
    assert(result4.size() == 0);
    std::cout << "  Empty range scan [35,38]: " << result4.size() << " pairs - PASSED" << std::endl;
    
    // Test 5: Range outside bounds
    auto result5 = sst.scan(100, 200, filename, true);
    assert(result5.size() == 0);
    std::cout << "  Out of bounds scan [100,200]: " << result5.size() << " pairs - PASSED" << std::endl;
    
    unlink(filename.c_str());
}

void testMultiLeafScan() {
    std::cout << "\n[Test 2: Multi-Leaf Scan]" << std::endl;
    
    // Create larger dataset spanning multiple leaves
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 20; i++) {
        data.emplace_back(i * 10, i * 100);
    }
    
    std::string filename = createTestSST(data);
    if (filename.empty()) {
        std::cerr << "  FAILED: Could not create test file" << std::endl;
        return;
    }
    
    BTreeSST sst;
    
    // Test 1: Full scan
    auto result1 = sst.scan(10, 200, filename, true);
    assert(result1.size() == 20);
    std::cout << "  Full scan [10,200]: " << result1.size() << " pairs - PASSED" << std::endl;
    
    // Test 2: Mid-range scan
    auto result2 = sst.scan(50, 150, filename, true);
    assert(result2.size() == 11);
    assert(result2[0].first == 50);
    assert(result2[10].first == 150);
    std::cout << "  Mid-range scan [50,150]: " << result2.size() << " pairs - PASSED" << std::endl;
    
    // Test 3: Scan from beginning
    auto result3 = sst.scan(10, 80, filename, true);
    assert(result3.size() == 8);
    std::cout << "  Start range scan [10,80]: " << result3.size() << " pairs - PASSED" << std::endl;
    
    // Test 4: Scan to end
    auto result4 = sst.scan(140, 200, filename, true);
    assert(result4.size() == 7);
    std::cout << "  End range scan [140,200]: " << result4.size() << " pairs - PASSED" << std::endl;
    
    unlink(filename.c_str());
}

void testLargeDatasetScan() {
    std::cout << "\n[Test 3: Large Dataset Scan]" << std::endl;
    
    // Create large dataset
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.emplace_back(i * 2, i * 20);
    }
    
    std::string filename = createTestSST(data);
    if (filename.empty()) {
        std::cerr << "  FAILED: Could not create test file" << std::endl;
        return;
    }
    
    BTreeSST sst;
    
    // Test 1: Large range scan
    auto result1 = sst.scan(2, 200, filename, true);
    assert(result1.size() == 100);
    std::cout << "  Full dataset scan [2,200]: " << result1.size() << " pairs - PASSED" << std::endl;
    
    // Test 2: Small range in large dataset
    auto result2 = sst.scan(50, 70, filename, true);
    assert(result2.size() == 11);
    std::cout << "  Small range scan [50,70]: " << result2.size() << " pairs - PASSED" << std::endl;
    
    // Test 3: Verify correct values
    auto result3 = sst.scan(100, 120, filename, true);
    assert(result3.size() == 11);
    assert(result3[0].first == 100 && result3[0].second == 1000);
    assert(result3[10].first == 120 && result3[10].second == 1200);
    std::cout << "  Value verification scan [100,120]: PASSED" << std::endl;
    
    unlink(filename.c_str());
}

void testEdgeCases() {
    std::cout << "\n[Test 4: Edge Cases]" << std::endl;
    
    // Test 1: Invalid range (key1 > key2)
    {
        std::vector<std::pair<int, int>> data = {{10, 100}, {20, 200}, {30, 300}};
        std::string filename = createTestSST(data);
        BTreeSST sst;
        
        auto result = sst.scan(30, 10, filename, true);
        assert(result.size() == 0);
        std::cout << "  Invalid range [30,10]: PASSED" << std::endl;
        unlink(filename.c_str());
    }
    
    // Test 2: Single element dataset
    {
        std::vector<std::pair<int, int>> data = {{42, 420}};
        std::string filename = createTestSST(data);
        BTreeSST sst;
        
        auto result1 = sst.scan(42, 42, filename, true);
        assert(result1.size() == 1);
        assert(result1[0].first == 42 && result1[0].second == 420);
        
        auto result2 = sst.scan(1, 100, filename, true);
        assert(result2.size() == 1);
        
        auto result3 = sst.scan(1, 40, filename, true);
        assert(result3.size() == 0);
        
        std::cout << "  Single element dataset: PASSED" << std::endl;
        unlink(filename.c_str());
    }
    
    // Test 3: Range extends beyond data bounds
    {
        std::vector<std::pair<int, int>> data = {{10, 100}, {20, 200}, {30, 300}};
        std::string filename = createTestSST(data);
        BTreeSST sst;
        
        auto result1 = sst.scan(5, 35, filename, true);
        assert(result1.size() == 3);
        
        auto result2 = sst.scan(15, 100, filename, true);
        assert(result2.size() == 2);
        
        auto result3 = sst.scan(1, 1000, filename, true);
        assert(result3.size() == 3);
        
        std::cout << "  Extended range: PASSED" << std::endl;
        unlink(filename.c_str());
    }
    
    // Test 4: Gaps in data
    {
        std::vector<std::pair<int, int>> data = {{10, 100}, {30, 300}, {50, 500}, {70, 700}};
        std::string filename = createTestSST(data);
        BTreeSST sst;
        
        auto result = sst.scan(20, 60, filename, true);
        assert(result.size() == 2); // Only 30 and 50
        assert(result[0].first == 30);
        assert(result[1].first == 50);
        
        std::cout << "  Gaps in data: PASSED" << std::endl;
        unlink(filename.c_str());
    }
}

void testBoundaryConditions() {
    std::cout << "\n[Test 5: Boundary Conditions]" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 50; i++) {
        data.emplace_back(i * 10, i * 100);
    }
    
    std::string filename = createTestSST(data);
    BTreeSST sst;
    
    // Test exact min/max
    auto result1 = sst.scan(10, 10, filename, true);
    assert(result1.size() == 1);
    assert(result1[0].first == 10);
    std::cout << "  Exact min boundary: PASSED" << std::endl;
    
    auto result2 = sst.scan(500, 500, filename, true);
    assert(result2.size() == 1);
    assert(result2[0].first == 500);
    std::cout << "  Exact max boundary: PASSED" << std::endl;
    
    // Test one before min
    auto result3 = sst.scan(5, 9, filename, true);
    assert(result3.size() == 0);
    std::cout << "  Before min boundary: PASSED" << std::endl;
    
    // Test one after max
    auto result4 = sst.scan(501, 600, filename, true);
    assert(result4.size() == 0);
    std::cout << "  After max boundary: PASSED" << std::endl;
    
    unlink(filename.c_str());
}

void testVeryLargeDatasetScan() {
    std::cout << "\n[Test 6: Very Large Dataset Scan (256,000 entries)]" << std::endl;
    
    // Create a very large dataset that will require 3+ internal levels
    std::vector<std::pair<int, int>> data;
    data.reserve(256000);
    for (int i = 1; i <= 256000; i++) {
        data.emplace_back(i, i * 10);
    }
    
    std::cout << "  Creating SST with " << data.size() << " entries..." << std::endl;
    std::string filename = createTestSST(data);
    if (filename.empty()) {
        std::cerr << "  FAILED: Could not create test file" << std::endl;
        return;
    }
    
    BTreeSST sst;
    
    // The tree will have multiple internal levels with this many entries
    // With PAGE_SIZE=128, MAX_LEAF_PAIRS=16: 256000/16 = 16000 leaves
    // This requires 3+ internal levels to index
    std::cout << "  Tree will have 16,000 leaf pages requiring 3+ internal levels" << std::endl;
    
    // Test 1: Full range scan
    std::cout << "  Testing full range scan..." << std::endl;
    auto result1 = sst.scan(1, 256000, filename, true);
    assert(result1.size() == 256000);
    assert(result1[0].first == 1 && result1[0].second == 10);
    assert(result1[255999].first == 256000 && result1[255999].second == 2560000);
    std::cout << "    Full range scan [1,256000]: " << result1.size() << " pairs - PASSED" << std::endl;
    
    // Test 2: Large mid-range scan (50,000 entries)
    std::cout << "  Testing large mid-range scan..." << std::endl;
    auto result2 = sst.scan(100000, 150000, filename, true);
    assert(result2.size() == 50001); // inclusive range
    assert(result2[0].first == 100000 && result2[0].second == 1000000);
    assert(result2[50000].first == 150000 && result2[50000].second == 1500000);
    std::cout << "    Mid-range scan [100000,150000]: " << result2.size() << " pairs - PASSED" << std::endl;
    
    // Test 3: Small range in large dataset (100 entries)
    std::cout << "  Testing small range in large dataset..." << std::endl;
    auto result3 = sst.scan(128000, 128100, filename, true);
    assert(result3.size() == 101);
    assert(result3[0].first == 128000);
    assert(result3[100].first == 128100);
    std::cout << "    Small range scan [128000,128100]: " << result3.size() << " pairs - PASSED" << std::endl;
    
    // Test 4: Beginning range
    std::cout << "  Testing beginning range..." << std::endl;
    auto result4 = sst.scan(1, 1000, filename, true);
    assert(result4.size() == 1000);
    assert(result4[0].first == 1 && result4[0].second == 10);
    assert(result4[999].first == 1000 && result4[999].second == 10000);
    std::cout << "    Beginning range scan [1,1000]: " << result4.size() << " pairs - PASSED" << std::endl;
    
    // Test 5: End range
    std::cout << "  Testing end range..." << std::endl;
    auto result5 = sst.scan(255000, 256000, filename, true);
    assert(result5.size() == 1001);
    assert(result5[0].first == 255000 && result5[0].second == 2550000);
    assert(result5[1000].first == 256000 && result5[1000].second == 2560000);
    std::cout << "    End range scan [255000,256000]: " << result5.size() << " pairs - PASSED" << std::endl;
    
    // Test 6: Single key in large dataset
    std::cout << "  Testing single key lookup..." << std::endl;
    auto result6 = sst.scan(128000, 128000, filename, true);
    assert(result6.size() == 1);
    assert(result6[0].first == 128000 && result6[0].second == 1280000);
    std::cout << "    Single key scan [128000,128000]: PASSED" << std::endl;
    
    // Test 7: Non-existent range (outside bounds)
    std::cout << "  Testing out-of-bounds ranges..." << std::endl;
    auto result7 = sst.scan(0, 0, filename, true);
    assert(result7.size() == 0);
    auto result8 = sst.scan(300000, 400000, filename, true);
    assert(result8.size() == 0);
    std::cout << "    Out-of-bounds scans: PASSED" << std::endl;
    
    // Test 8: Verify sorted order in result
    std::cout << "  Verifying sorted order in large result..." << std::endl;
    auto result9 = sst.scan(50000, 51000, filename, true);
    assert(result9.size() == 1001);
    for (size_t i = 1; i < result9.size(); i++) {
        assert(result9[i].first > result9[i-1].first); // Strictly increasing
    }
    std::cout << "    Sorted order verification: PASSED" << std::endl;
    
    // Test 9: Quarter, half, three-quarter points
    std::cout << "  Testing specific range queries..." << std::endl;
    auto result10 = sst.scan(64000, 64000, filename, true);  // Quarter point
    assert(result10.size() == 1 && result10[0].first == 64000);
    
    auto result11 = sst.scan(192000, 192000, filename, true); // Three-quarter point
    assert(result11.size() == 1 && result11[0].first == 192000);
    std::cout << "    Specific point queries: PASSED" << std::endl;
    
    unlink(filename.c_str());
    
    std::cout << "  All very large dataset scan tests PASSED!" << std::endl;
}

} // anonymous namespace

int main() {
    std::cout << "╔═══════════════════════════════════════╗" << std::endl;
    std::cout << "║  B-Tree scan() Tests                  ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════╝" << std::endl;
    
    try {
        testSingleLeafScan();
        testMultiLeafScan();
        testLargeDatasetScan();
        testEdgeCases();
        testBoundaryConditions();
        testVeryLargeDatasetScan();
        
        std::cout << "\n╔═══════════════════════════════════════╗" << std::endl;
        std::cout << "║  All scan() tests PASSED!             ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════╝" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
