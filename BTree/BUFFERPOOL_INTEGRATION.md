# BufferPool Integration in BTreeSST

## Overview

The `BTreeSST` class now supports optional BufferPool integration for efficient page caching. All `pread` operations during queries (`get`, `scan`, `toSortedArray`) now use the BufferPool when available, reducing disk I/O.

## Changes Made

### 1. Updated BTreeSST Class

**Header (`BTreeSST.hpp`):**
- Added `BufferPool* bufferPool` member (non-owning pointer)
- Updated constructor: `BTreeSST(BufferPool* pool = nullptr)`
- Added `setBufferPool(BufferPool* pool)` method
- Added helper functions:
  - `readPages()` - Read multiple consecutive pages
  - `readBytesBuffered()` - Read raw bytes using page-aligned buffer pool reads

**Implementation (`BTreeSST.cpp`):**
- `readPage()` now checks if BufferPool is available:
  - If yes: Check cache → Read from disk if miss → Add to cache
  - If no: Fall back to direct `pread`
- All read operations (`get`, `scan`, `toSortedArray`) now use `readBytesBuffered()`
- Build operations (`buildLeafNodes`, `buildInternalLevels`) still use direct `pwrite` for bulk sequential writes (more efficient)

### 2. Updated Makefile

Added all BufferPool dependencies to `BTREE_SOURCES`:
```makefile
BTREE_SOURCES = BTree/BTreeSST.cpp BufferPool/Page.cpp BufferPool/PageID.cpp \
    BufferPool/MurmurHash.cpp BufferPool/BucketNode.cpp BufferPool/Bucket.cpp \
    BufferPool/HashTable.cpp BufferPool/BufferPool.cpp BufferPool/ClockEvictionPolicy.cpp
```

## Usage

### Without BufferPool (Backward Compatible)

```cpp
BTreeSST sst;  // No buffer pool - uses direct I/O
std::vector<std::pair<int, int>> data = {{1, 10}, {2, 20}, {3, 30}};
sst.buildBTree(data, "test.sst");

int value;
bool found = sst.get(2, value, "test.sst");  // Direct pread
```

### With BufferPool (Recommended)

```cpp
#include "BTree/BTreeSST.hpp"
#include "BufferPool/BufferPool.hpp"
#include "BufferPool/ClockEvictionPolicy.hpp"

// Create buffer pool (e.g., 1000 pages = ~4MB)
auto evictionPolicy = std::make_unique<ClockEvictionPolicy>();
BufferPool bufferPool(1000, std::move(evictionPolicy));

// Create BTreeSST with buffer pool
BTreeSST sst(&bufferPool);

// Build SST (writes still use direct I/O for efficiency)
std::vector<std::pair<int, int>> data;
for (int i = 0; i < 10000; i++) {
    data.push_back({i, i * 10});
}
sst.buildBTree(data, "large.sst");

// Queries now use buffer pool
int value;
bool found = sst.get(5000, value, "large.sst");  // Uses buffer pool

// Range scans benefit from page caching
auto results = sst.scan(1000, 2000, "large.sst");  // Cached pages reused

// Convert to array also uses buffer pool
auto allData = sst.toSortedArray("large.sst");
```

### Shared BufferPool Across Multiple SSTs

```cpp
// Single buffer pool shared by multiple SST files
auto policy = std::make_unique<ClockEvictionPolicy>();
BufferPool sharedPool(5000, std::move(policy));

BTreeSST sst1(&sharedPool);
BTreeSST sst2(&sharedPool);
BTreeSST sst3(&sharedPool);

// All SSTs share the same page cache
sst1.buildBTree(data1, "sst1.sst");
sst2.buildBTree(data2, "sst2.sst");
sst3.buildBTree(data3, "sst3.sst");

// Frequently accessed pages from any SST stay in cache
int v1, v2, v3;
sst1.get(key, v1, "sst1.sst");  // May cache pages
sst2.get(key, v2, "sst2.sst");  // May evict sst1 pages if needed
sst1.get(key, v1, "sst1.sst");  // May hit cache if page still present
```

## Performance Benefits

1. **Reduced Disk I/O**: Frequently accessed pages are cached in memory
2. **Page Reuse**: Multiple reads of the same page hit the cache
3. **Range Scan Optimization**: Sequential scans benefit from cached pages
4. **Configurable Cache Size**: Adjust buffer pool size based on available memory
5. **Eviction Policy**: CLOCK algorithm ensures efficient cache utilization

## PageID Format

Pages are identified by `PageID(fileName, offset)`:
- `fileName`: Full path to the SST file
- `offset`: Byte offset in the file (pageId × PAGE_SIZE)

Example: Page 3 of `/tmp/data.sst` → `PageID("/tmp/data.sst", 12288)`

## Build vs Query Operations

| Operation | Uses BufferPool? | Reason |
|-----------|------------------|--------|
| `buildBTree()` | No (direct pwrite) | Bulk sequential writes are more efficient without caching |
| `get()` | Yes (if available) | Point queries benefit from caching |
| `scan()` | Yes (if available) | Range scans reuse cached pages |
| `toSortedArray()` | Yes (if available) | Full scan benefits from page caching |

## Memory Considerations

- Each cached page uses 4KB of memory
- BufferPool with 1000 pages ≈ 4MB
- BufferPool with 10000 pages ≈ 40MB
- Choose size based on working set and available RAM

## Backward Compatibility

All existing code continues to work without changes. The BufferPool integration is opt-in:
- Default constructor: `BTreeSST()` → No buffer pool
- With buffer pool: `BTreeSST(&bufferPool)` → Caching enabled
