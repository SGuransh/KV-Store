# Clock Eviction Policy Design

## Overview

The Clock Eviction Policy implements the Clock page replacement algorithm with **O(1) time complexity** for all three key operations: access recording, insertion, and removal. This is achieved through a fixed-size pre-allocated array and smart index management.

## Core Data Structures

### Fixed-Size Clock Array

```cpp
std::vector<ClockEntry> clockArray;  // Pre-allocated to buffer capacity
```

- **Size**: Fixed at buffer pool's maximum capacity
- **Purpose**: Eliminates dynamic resizing and provides predictable memory layout
- **Entry Structure**: `{PageID, referenceBit, isValid}`

### Index Management System

```cpp
std::unordered_map<std::string, std::size_t> pageIndex;  // PageID -> array index
std::vector<std::size_t> freeIndices;                   // Stack of reusable indices
std::size_t fillPointer;                                // Next position for initial fill
```

## O(1) Operations Breakdown

### 1. Record Access - O(1)

```cpp
void recordAccess(const PageID& id)
```

**Steps:**

1. Hash map lookup: `pageIndex.find(pageKey)` → O(1)
2. Direct array access: `clockArray[index].referenceBit = true` → O(1)

**Why O(1):** Single hash lookup + direct array indexing.

### 2. Record Insertion - O(1)

```cpp
void recordInsertion(const PageID& id)
```

**Steps:**

1. Get next available index → O(1)
   - If `freeIndices` not empty: pop from stack
   - Else: use `fillPointer++`
2. Direct array assignment: `clockArray[index] = ClockEntry(id)` → O(1)
3. Hash map insertion: `pageIndex[pageKey] = index` → O(1)

**Why O(1):** All operations are constant time - no array shifting or resizing.

### 3. Record Removal - O(1)

```cpp
void recordRemoval(const PageID& id)
```

**Steps:**

1. Hash map lookup: `pageIndex.find(pageKey)` → O(1)
2. Mark slot invalid: `clockArray[index].isValid = false` → O(1)
3. Add to free indices: `freeIndices.push_back(index)` → O(1)
4. Hash map removal: `pageIndex.erase(iterator)` → O(1)

**Why O(1):** No array element shifting - just mark as invalid and reuse later.

## Index Management Strategy

### Initial Fill Phase

- Use `fillPointer` to track next empty slot (0 → capacity)
- Simple sequential allocation: `clockArray[fillPointer++] = newEntry`

### Post-Removal Phase

- Maintain `freeIndices` stack of reusable slots
- Prioritize reusing freed slots over extending fill pointer
- Prevents fragmentation and maintains O(1) insertion

### Index Resolution

```cpp
std::size_t getNextAvailableIndex() {
    if (!freeIndices.empty()) {
        return freeIndices.back(); freeIndices.pop_back();  // Reuse freed slot
    }
    return fillPointer++;  // Use next sequential slot
}
```

## Clock Algorithm Preservation

### Circular Traversal

- Clock hand (`clockPointer`) moves through array circularly
- **Skip invalid entries**: `while (!clockArray[clockPointer].isValid)`
- Maintains algorithm fairness despite "holes" in array

### Reference Bit Management

- Set on access: `referenceBit = true`
- Cleared during clock sweep: `referenceBit = false` (second chance)
- Victim selection: first page with `referenceBit = false`

## Memory Efficiency

### Fixed Allocation Benefits

- **Predictable memory usage**: Exactly `capacity * sizeof(ClockEntry)`
- **No fragmentation**: Pre-allocated contiguous memory
- **Cache friendly**: Sequential access patterns during clock sweep

### Space Complexity

- Clock array: O(capacity)
- Page index map: O(current_pages)
- Free indices: O(removed_pages)
- **Total**: O(capacity) - optimal for buffer pool

## Performance Guarantees

| Operation           | Time Complexity   | Space Impact   |
| ------------------- | ----------------- | -------------- |
| `recordAccess()`    | O(1)              | None           |
| `recordInsertion()` | O(1)              | +1 valid entry |
| `recordRemoval()`   | O(1)              | +1 free index  |
| `selectVictim()`    | O(n) worst case\* | None           |

\*Clock sweep may visit all pages in worst case, but this is inherent to the algorithm and maintains fairness.

## Key Design Decisions

1. **Fixed vs Dynamic Array**: Chose fixed for O(1) guarantees and memory predictability
2. **Mark Invalid vs Compact**: Chose marking to avoid O(n) shifting operations
3. **Free Index Stack**: Enables O(1) reuse of freed slots
4. **Hash Map Indexing**: Provides O(1) PageID to array index translation

This design achieves optimal performance while preserving the Clock algorithm's correctness and fairness properties.
