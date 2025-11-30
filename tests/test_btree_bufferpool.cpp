#include "../BTree/BTreeSST.hpp"
#include "../BufferPool/BufferPool.hpp"
#include "../BufferPool/ClockEvictionPolicy.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <unistd.h>
#include <cstring>

// Test counter
int tests_passed = 0;
int tests_failed = 0;

void test_passed(const std::string& test_name) {
    tests_passed++;
    std::cout << "[PASS] " << test_name << std::endl;
}

void test_failed(const std::string& test_name, const std::string& reason = "") {
    tests_failed++;
    std::cout << "[FAIL] " << test_name;
    if (!reason.empty()) {
        std::cout << " - " << reason;
    }
    std::cout << std::endl;
}

// Custom BufferPool subclass to track get/put operations
class InstrumentedBufferPool : public BufferPool {
private:
    size_t getCallCount = 0;
    size_t putCallCount = 0;
    size_t hitCount = 0;
    size_t missCount = 0;

public:
    InstrumentedBufferPool(size_t bufferSize, std::unique_ptr<EvictionPolicy> policy)
        : BufferPool(bufferSize, std::move(policy)) {}

    Page* getPage(const PageID& id) {
        getCallCount++;
        Page* result = BufferPool::getPage(id);
        if (result != nullptr) {
            hitCount++;
        } else {
            missCount++;
        }
        return result;
    }

    bool putPage(const PageID& id, const Page& pageData) {
        putCallCount++;
        return BufferPool::putPage(id, pageData);
    }

    // Getters for statistics
    size_t getGetCallCount() const { return getCallCount; }
    size_t getPutCallCount() const { return putCallCount; }
    size_t getHitCount() const { return hitCount; }
    size_t getMissCount() const { return missCount; }
    double getHitRate() const {
        return getCallCount > 0 ? (double)hitCount / getCallCount : 0.0;
    }

    void resetStats() {
        getCallCount = 0;
        putCallCount = 0;
        hitCount = 0;
        missCount = 0;
    }
};

// Helper to create a test SST file
std::string createTestSST(const std::vector<std::pair<int, int>>& data, const std::string& suffix = "") {
    char tmpTemplate[256];
    snprintf(tmpTemplate, sizeof(tmpTemplate), "/tmp/btree_bp_test_%s_XXXXXX", suffix.c_str());
    int fd = mkstemp(tmpTemplate);
    if (fd < 0) {
        std::cerr << "Failed to create temporary file" << std::endl;
        return "";
    }
    close(fd);
    
    BTreeSST sst;  // Build without buffer pool (direct I/O)
    if (!sst.buildBTree(data, tmpTemplate)) {
        std::cerr << "Failed to build B-Tree" << std::endl;
        return "";
    }
    
    return std::string(tmpTemplate);
}

/**
 * Test 1: Verify BufferPool is used for get() operations
 */
bool test_bufferpool_used_for_get() {
    std::cout << "\n=== Test 1: BufferPool Used for get() ===" << std::endl;
    
    // Create test data
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 1000; i++) {
        data.push_back({i, i * 10});
    }
    
    std::string filename = createTestSST(data, "get");
    if (filename.empty()) {
        test_failed("BufferPool get() - SST creation failed");
        return false;
    }
    
    // Create instrumented buffer pool
    auto policy = std::make_unique<ClockEvictionPolicy>();
    InstrumentedBufferPool pool(100, std::move(policy));
    
    // Create BTreeSST with buffer pool
    BTreeSST sst(&pool);
    
    // Perform get operations
    int value;
    bool found = sst.get(500, value, filename);
    
    if (!found || value != 5000) {
        test_failed("BufferPool get()", "Query failed");
        unlink(filename.c_str());
        return false;
    }
    
    // Verify buffer pool was used
    if (pool.getGetCallCount() == 0) {
        test_failed("BufferPool get()", "BufferPool.getPage() never called");
        unlink(filename.c_str());
        return false;
    }
    
    if (pool.getPutCallCount() == 0) {
        test_failed("BufferPool get()", "BufferPool.putPage() never called");
        unlink(filename.c_str());
        return false;
    }
    
    std::cout << "  BufferPool stats:" << std::endl;
    std::cout << "    getPage() calls: " << pool.getGetCallCount() << std::endl;
    std::cout << "    putPage() calls: " << pool.getPutCallCount() << std::endl;
    std::cout << "    Cache hits: " << pool.getHitCount() << std::endl;
    std::cout << "    Cache misses: " << pool.getMissCount() << std::endl;
    
    unlink(filename.c_str());
    test_passed("BufferPool get()");
    return true;
}

/**
 * Test 2: Verify cache hits on repeated get() operations
 */
bool test_cache_hits_on_repeated_get() {
    std::cout << "\n=== Test 2: Cache Hits on Repeated get() ===" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i, i * 10});
    }
    
    std::string filename = createTestSST(data, "repeat");
    if (filename.empty()) {
        test_failed("Cache hits", "SST creation failed");
        return false;
    }
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    InstrumentedBufferPool pool(50, std::move(policy));
    BTreeSST sst(&pool);
    
    // First query - should miss cache
    int value;
    sst.get(50, value, filename);
    size_t firstMisses = pool.getMissCount();
    
    pool.resetStats();
    
    // Second query for same key - should hit cache
    sst.get(50, value, filename);
    size_t secondHits = pool.getHitCount();
    
    if (secondHits == 0) {
        test_failed("Cache hits", "No cache hits on repeated query");
        std::cout << "    Hits: " << secondHits << ", Misses: " << pool.getMissCount() << std::endl;
        unlink(filename.c_str());
        return false;
    }
    
    std::cout << "  First query - Misses: " << firstMisses << std::endl;
    std::cout << "  Second query - Hits: " << secondHits << ", Misses: " << pool.getMissCount() << std::endl;
    
    unlink(filename.c_str());
    test_passed("Cache hits on repeated get()");
    return true;
}

/**
 * Test 3: Verify BufferPool is used for scan() operations
 */
bool test_bufferpool_used_for_scan() {
    std::cout << "\n=== Test 3: BufferPool Used for scan() ===" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 1000; i++) {
        data.push_back({i, i * 10});
    }
    
    std::string filename = createTestSST(data, "scan");
    if (filename.empty()) {
        test_failed("BufferPool scan()", "SST creation failed");
        return false;
    }
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    InstrumentedBufferPool pool(100, std::move(policy));
    BTreeSST sst(&pool);
    
    // Perform scan
    auto results = sst.scan(100, 200, filename);
    
    if (results.size() != 101) {
        test_failed("BufferPool scan()", "Wrong result count");
        unlink(filename.c_str());
        return false;
    }
    
    // Verify buffer pool was used
    if (pool.getGetCallCount() == 0) {
        test_failed("BufferPool scan()", "BufferPool.getPage() never called");
        unlink(filename.c_str());
        return false;
    }
    
    std::cout << "  BufferPool stats:" << std::endl;
    std::cout << "    getPage() calls: " << pool.getGetCallCount() << std::endl;
    std::cout << "    putPage() calls: " << pool.getPutCallCount() << std::endl;
    std::cout << "    Hit rate: " << (pool.getHitRate() * 100) << "%" << std::endl;
    
    unlink(filename.c_str());
    test_passed("BufferPool scan()");
    return true;
}

/**
 * Test 4: Verify BufferPool is used for toSortedArray()
 */
bool test_bufferpool_used_for_sorted_array() {
    std::cout << "\n=== Test 4: BufferPool Used for toSortedArray() ===" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 500; i++) {
        data.push_back({i, i * 10});
    }
    
    std::string filename = createTestSST(data, "array");
    if (filename.empty()) {
        test_failed("BufferPool toSortedArray()", "SST creation failed");
        return false;
    }
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    InstrumentedBufferPool pool(100, std::move(policy));
    BTreeSST sst(&pool);
    
    // Convert to sorted array
    auto results = sst.toSortedArray(filename);
    
    if (results.size() != 500) {
        test_failed("BufferPool toSortedArray()", "Wrong result count");
        unlink(filename.c_str());
        return false;
    }
    
    // Verify buffer pool was used
    if (pool.getGetCallCount() == 0) {
        test_failed("BufferPool toSortedArray()", "BufferPool.getPage() never called");
        unlink(filename.c_str());
        return false;
    }
    
    std::cout << "  BufferPool stats:" << std::endl;
    std::cout << "    getPage() calls: " << pool.getGetCallCount() << std::endl;
    std::cout << "    putPage() calls: " << pool.getPutCallCount() << std::endl;
    
    unlink(filename.c_str());
    test_passed("BufferPool toSortedArray()");
    return true;
}

/**
 * Test 5: Verify no BufferPool calls when pool is nullptr
 */
bool test_no_bufferpool_when_null() {
    std::cout << "\n=== Test 5: No BufferPool When Null ===" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i, i * 10});
    }
    
    std::string filename = createTestSST(data, "null");
    if (filename.empty()) {
        test_failed("No BufferPool", "SST creation failed");
        return false;
    }
    
    // Create BTreeSST WITHOUT buffer pool
    BTreeSST sst;  // nullptr buffer pool
    
    // Perform operations
    int value;
    bool found = sst.get(50, value, filename);
    
    if (!found || value != 500) {
        test_failed("No BufferPool", "Query failed");
        unlink(filename.c_str());
        return false;
    }
    
    auto results = sst.scan(10, 20, filename);
    if (results.size() != 11) {
        test_failed("No BufferPool", "Scan failed");
        unlink(filename.c_str());
        return false;
    }
    
    std::cout << "  Operations completed successfully without BufferPool" << std::endl;
    
    unlink(filename.c_str());
    test_passed("No BufferPool when null");
    return true;
}

/**
 * Test 6: Verify cache efficiency with large dataset
 */
bool test_cache_efficiency_large_dataset() {
    std::cout << "\n=== Test 6: Cache Efficiency Large Dataset ===" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 10000; i++) {
        data.push_back({i, i * 10});
    }
    
    std::string filename = createTestSST(data, "large");
    if (filename.empty()) {
        test_failed("Cache efficiency", "SST creation failed");
        return false;
    }
    
    auto policy = std::make_unique<ClockEvictionPolicy>();
    InstrumentedBufferPool pool(200, std::move(policy));  // ~800KB cache
    BTreeSST sst(&pool);
    
    // Query same region multiple times
    for (int iteration = 0; iteration < 3; iteration++) {
        auto results = sst.scan(1000, 2000, filename);
        if (results.size() != 1001) {
            test_failed("Cache efficiency", "Scan returned wrong count");
            unlink(filename.c_str());
            return false;
        }
    }
    
    double hitRate = pool.getHitRate();
    std::cout << "  After 3 scans of same range:" << std::endl;
    std::cout << "    Total getPage() calls: " << pool.getGetCallCount() << std::endl;
    std::cout << "    Cache hits: " << pool.getHitCount() << std::endl;
    std::cout << "    Cache misses: " << pool.getMissCount() << std::endl;
    std::cout << "    Hit rate: " << (hitRate * 100) << "%" << std::endl;
    
    // After 3 iterations, we should have good hit rate
    if (hitRate < 0.50) {  // At least 50% hit rate
        test_failed("Cache efficiency", "Hit rate too low: " + std::to_string(hitRate * 100) + "%");
        unlink(filename.c_str());
        return false;
    }
    
    unlink(filename.c_str());
    test_passed("Cache efficiency");
    return true;
}

/**
 * Test 7: Verify shared BufferPool across multiple SSTs
 */
bool test_shared_bufferpool_multiple_ssts() {
    std::cout << "\n=== Test 7: Shared BufferPool Multiple SSTs ===" << std::endl;
    
    // Create multiple SST files
    std::vector<std::pair<int, int>> data1, data2, data3;
    for (int i = 1; i <= 100; i++) {
        data1.push_back({i, i * 10});
        data2.push_back({i + 1000, i * 20});
        data3.push_back({i + 2000, i * 30});
    }
    
    std::string file1 = createTestSST(data1, "sst1");
    std::string file2 = createTestSST(data2, "sst2");
    std::string file3 = createTestSST(data3, "sst3");
    
    if (file1.empty() || file2.empty() || file3.empty()) {
        test_failed("Shared BufferPool", "SST creation failed");
        return false;
    }
    
    // Create shared buffer pool
    auto policy = std::make_unique<ClockEvictionPolicy>();
    InstrumentedBufferPool sharedPool(100, std::move(policy));
    
    // Create multiple BTreeSST instances sharing the pool
    BTreeSST sst1(&sharedPool);
    BTreeSST sst2(&sharedPool);
    BTreeSST sst3(&sharedPool);
    
    // Query all three SSTs
    int v1, v2, v3;
    sst1.get(50, v1, file1);
    sst2.get(1050, v2, file2);
    sst3.get(2050, v3, file3);
    
    if (v1 != 500 || v2 != 1000 || v3 != 1500) {
        test_failed("Shared BufferPool", "Queries returned wrong values");
        unlink(file1.c_str());
        unlink(file2.c_str());
        unlink(file3.c_str());
        return false;
    }
    
    // Verify all used the same pool
    if (sharedPool.getGetCallCount() == 0) {
        test_failed("Shared BufferPool", "BufferPool not used");
        unlink(file1.c_str());
        unlink(file2.c_str());
        unlink(file3.c_str());
        return false;
    }
    
    std::cout << "  Shared BufferPool stats:" << std::endl;
    std::cout << "    Total getPage() calls: " << sharedPool.getGetCallCount() << std::endl;
    std::cout << "    Total putPage() calls: " << sharedPool.getPutCallCount() << std::endl;
    std::cout << "    Current cache size: " << sharedPool.getCurrentSize() << std::endl;
    
    unlink(file1.c_str());
    unlink(file2.c_str());
    unlink(file3.c_str());
    test_passed("Shared BufferPool multiple SSTs");
    return true;
}

/**
 * Test 8: Verify eviction works correctly
 */
bool test_bufferpool_eviction() {
    std::cout << "\n=== Test 8: BufferPool Eviction ===" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 1000; i++) {
        data.push_back({i, i * 10});
    }
    
    std::string filename = createTestSST(data, "evict");
    if (filename.empty()) {
        test_failed("BufferPool eviction", "SST creation failed");
        return false;
    }
    
    // Create small buffer pool to force evictions
    auto policy = std::make_unique<ClockEvictionPolicy>();
    InstrumentedBufferPool pool(5, std::move(policy));  // Only 5 pages
    BTreeSST sst(&pool);
    
    // Perform operations that require more than 5 pages
    for (int i = 0; i < 10; i++) {
        int value;
        sst.get(i * 100 + 1, value, filename);
    }
    
    size_t finalCacheSize = pool.getCurrentSize();
    
    // Cache size should not exceed buffer pool size
    if (finalCacheSize > 5) {
        test_failed("BufferPool eviction", "Cache exceeded buffer size: " + std::to_string(finalCacheSize));
        unlink(filename.c_str());
        return false;
    }
    
    std::cout << "  After 10 queries with 5-page buffer:" << std::endl;
    std::cout << "    Cache size: " << finalCacheSize << " (max: 5)" << std::endl;
    std::cout << "    getPage() calls: " << pool.getGetCallCount() << std::endl;
    std::cout << "    Evictions occurred: " << (pool.getPutCallCount() > 5 ? "Yes" : "No") << std::endl;
    
    unlink(filename.c_str());
    test_passed("BufferPool eviction");
    return true;
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  B-Tree BufferPool Integration Tests             ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════╝" << std::endl;
    
    test_bufferpool_used_for_get();
    test_cache_hits_on_repeated_get();
    test_bufferpool_used_for_scan();
    test_bufferpool_used_for_sorted_array();
    test_no_bufferpool_when_null();
    test_cache_efficiency_large_dataset();
    test_shared_bufferpool_multiple_ssts();
    test_bufferpool_eviction();
    
    std::cout << "\n╔═══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Test Summary                                     ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════╝" << std::endl;
    std::cout << "Tests Passed: " << tests_passed << std::endl;
    std::cout << "Tests Failed: " << tests_failed << std::endl;
    
    if (tests_failed == 0) {
        std::cout << "\n✅ All BufferPool integration tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some tests FAILED!" << std::endl;
        return 1;
    }
}
