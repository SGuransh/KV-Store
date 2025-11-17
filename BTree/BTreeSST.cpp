#include "BTreeSST.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

/**
 * Main entry point: Build B-Tree SST from sorted data and write to disk
 */
bool BTreeSST::buildBTree(const std::vector<std::pair<int, int>>& sortedData, const std::string& fileName) {
    if (sortedData.empty()) {
        std::cerr << "Error: Cannot build B-Tree from empty data" << std::endl;
        return false;
    }
    
    std::cout << "Building B-Tree SST with " << sortedData.size() << " key-value pairs..." << std::endl;
    
    // Create build context
    BuildContext ctx(fileName);
    
    // Phase 1: Calculate sizes and allocate memory
    calculateAndAllocateArrays(ctx, sortedData.size());
    
    // Phase 2: Build leaf nodes from sorted data
    int32_t* max_per_node = buildLeafNodes(ctx, sortedData);
    
    // Calculate lastLeafPairs
    size_t lastLeafPairs = sortedData.size() % MAX_LEAF_PAIRS;
    if (lastLeafPairs == 0 && sortedData.size() > 0) {
        lastLeafPairs = MAX_LEAF_PAIRS;  // Last leaf is full
    }
    ctx.lastLeafPairs = lastLeafPairs;
    
    // Phase 3: Build internal levels bottom-up (only if we have > 1 leaf)
    if (ctx.leafNodeCount > 1) {
        buildInternalLevels(ctx, max_per_node);
        ctx.treeHeight = ctx.internalLevelCount + 1;  // Internal levels + leaf level
    } else {
        ctx.treeHeight = 1;  // Single leaf root
    }

    // Phase 4: Write metadata to disk
    bool success = writeMetadata(ctx);
    
    return success;
}

/**
 * Calculate array sizes and allocate memory
 */
void BTreeSST::calculateAndAllocateArrays(BuildContext& ctx, size_t dataSize) {
    // Calculate number of leaf nodes needed
    ctx.leafNodeCount = (dataSize + MAX_LEAF_PAIRS - 1) / MAX_LEAF_PAIRS;
    std::cout << "  Leaf nodes: " << ctx.leafNodeCount << " (capacity: " << MAX_LEAF_PAIRS << " pairs each)" << std::endl;
    
    // Open file for writing (will write leaves directly)
    ctx.fd = open(ctx.fileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (ctx.fd < 0) {
        std::cerr << "Error: Cannot open file for writing: " << ctx.fileName << std::endl;
        return;
    }
    
    // Calculate tree height (how many internal levels we need)
    if (ctx.leafNodeCount == 1) {
        // Special case: only 1 leaf, it becomes the root (no internal nodes)
        ctx.internalLevelCount = 0;
        ctx.totalInternalNodes = 0;
        std::cout << "  Tree height: 1 (single leaf root)" << std::endl;
        return;
    }
    else{
        // There would be atleast one internal level
        std::vector<size_t> levelSizes;
        size_t currentLevelSize = ctx.leafNodeCount;
        size_t totalInternalNodes = 0;
        std::vector<size_t> values_in_last_nodes;
        
        while (currentLevelSize > 1) {
            // Calculate parent level size: ceil(currentLevelSize / MAX_INTERNAL_CHILDREN)
            currentLevelSize = (currentLevelSize + MAX_INTERNAL_CHILDREN - 1) / MAX_INTERNAL_CHILDREN;
            totalInternalNodes += currentLevelSize;
            levelSizes.push_back(currentLevelSize);

            // Calculate number of keys in last node at this level
            size_t full_nodes = ctx.leafNodeCount / MAX_INTERNAL_CHILDREN;
            size_t remainder = ctx.leafNodeCount % MAX_INTERNAL_CHILDREN;
            if (remainder > 0) {
                values_in_last_nodes.push_back(remainder - 1);
            } else {
                values_in_last_nodes.push_back(MAX_INTERNAL_KEYS);
            }
        }
        
        ctx.internalLevelCount = levelSizes.size();
        ctx.totalInternalNodes = totalInternalNodes; 
        std::cout << "  Internal levels: " << ctx.internalLevelCount << std::endl;
        
        // Allocate internal level sizes
        ctx.internalLevelSizes = new size_t[ctx.internalLevelCount];
        
        for (size_t i = 0; i < ctx.internalLevelCount; i++) {
            ctx.internalLevelSizes[i] = levelSizes[i];
            std::cout << "    Level " << i << ": " << levelSizes[i] << " nodes" << std::endl;
        }
        
        std::cout << "  Tree height: " << (ctx.internalLevelCount + 1) << std::endl;

        // Allocate and populate nodesPerLevel (reversed: root first)
        //! This stores the number of nodes at each internal level in top-down order
        ctx.nodesPerLevel = new uint32_t[ctx.internalLevelCount];
        for (size_t i = 0; i < ctx.internalLevelCount; i++) {
            ctx.nodesPerLevel[i] = levelSizes[ctx.internalLevelCount - 1 - i];
        }
        
        //! This stores the number of keys in the last node at the internal levels have
        ctx.lastNodeKeys = new uint32_t[ctx.internalLevelCount];
        for (size_t i = 0; i < ctx.internalLevelCount; i++) {
            ctx.lastNodeKeys[i] = static_cast<uint32_t>(values_in_last_nodes[ctx.internalLevelCount - 1 - i]);
        }
        ctx.treeHeight = ctx.internalLevelCount + 1;

    }
}

/**
 * Phase 1: Write leaf nodes directly to disk
 * OPTIMIZED: Convert to interleaved array and write directly to disk pages
 * This skips the intermediate in-memory LeafNode array completely!
 */
int32_t* BTreeSST::buildLeafNodes(BuildContext& ctx, const std::vector<std::pair<int, int>>& sortedData) {
    std::cout << "Writing leaf nodes directly to disk..." << std::endl;
    
    // Set min and max keys from sorted data
    ctx.minKey = sortedData.front().first;
    ctx.maxKey = sortedData.back().first;
    
    // Convert sortedData to interleaved array [k1,v1, k2,v2, ...]
    size_t totalElements = sortedData.size() * 2;  // Each pair becomes 2 elements
    int32_t* interleavedData = new int32_t[totalElements];
    int32_t* max_per_node = new int32_t[ctx.leafNodeCount];
    int max = 0;
    int page = 0;
    
    for (size_t i = 0; i < sortedData.size(); i++) {
        interleavedData[i * 2] = sortedData[i].first;      // key
        interleavedData[i * 2 + 1] = sortedData[i].second; // value
        if (sortedData[i].first > max) {
            max = sortedData[i].first;
        }
        if (i % MAX_LEAF_PAIRS == MAX_LEAF_PAIRS - 1) {
            max_per_node[page] = max;
            max = 0;
            page++;
        }
    }
    // Handle last leaf's max if not a full leaf
    if (sortedData.size() % MAX_LEAF_PAIRS != 0) {
        max_per_node[page] = max;
    }

    ssize_t written = pwrite(ctx.fd, interleavedData, totalElements * sizeof(int32_t), (1 + ctx.totalInternalNodes) * Page::PAGE_SIZE);
    delete[] interleavedData;
    
    std::cout << "  All " << ctx.leafNodeCount << " leaf pages written directly to disk" << std::endl;
    return max_per_node;
}

/**
 * Phase 2: Build internal levels bottom-up
 * Uses "Largest Key" approach: keys[i] = largest key in children[i]
 * Uses sequential children optimization: no children array, just firstChild + count
 * OPTIMIZED: Read largest keys from disk instead of memory
 */
void BTreeSST::buildInternalLevels(BuildContext& ctx, const int32_t* max_per_node) {
    /*
     * Input: 
     *   - max_per_node: Array of maximum keys in each leaf node
     * Algorithm:
     *   1. Maintain a dynamic array of max-values to be processed for the current level.
     *   2. Group values by the B-tree fan-out (MAX_INTERNAL_CHILDREN) to populate internal nodes.
     *      The last node in a level may have fewer children; we store X - 1 keys when X children
     *      produce X maxima.
     *   3. Keep all internal nodes in a contiguous buffer sized by ctx.totalInternalNodes.
     *   4. Iterate levels bottom-up, computing offsets using ctx.internalLevelSizes so that pages
     *      are laid out level by level.
     *   5. Flush the contiguous buffer to disk starting at Page::PAGE_SIZE via pwrite and ctx.fd.
     *
     * Dry run example (B = 4):
     * 3 6 9 12 15 18 21 24 27 30 33 36 39 42 45 48 51 54 57 60
     * (3,6,9) (15, 18, 21) (27,30,33) (39,42,45) (51,54,57)
     * (12, 24, 36) ()
     * (48)
     */
    std::cout << "Building internal levels bottom-up..." << std::endl;

    // Maintain a dynamic array of max-values to be processed
    std::vector<int32_t> currentLevel(max_per_node, max_per_node + ctx.leafNodeCount);
    
    // Allocate contiguous array for all internal nodes
    int32_t* allInternalNodes = new int32_t[ctx.totalInternalNodes * MAX_INTERNAL_KEYS];
    std::memset(allInternalNodes, 0, ctx.totalInternalNodes * MAX_INTERNAL_KEYS * sizeof(int32_t));

    // Define next write index to compute the start offset for each level
    size_t nextWriteIndex = ctx.totalInternalNodes;

    // Iterate across required number of internal levels
    for (size_t levelIdx = 0; levelIdx < ctx.internalLevelCount; ++levelIdx) {
        size_t numNodesInLevel = ctx.internalLevelSizes[levelIdx];
        size_t levelStartIndex = nextWriteIndex - numNodesInLevel;
        std::vector<int32_t> nextLevel;
        nextLevel.reserve(numNodesInLevel);
        const size_t currentLevelSize = currentLevel.size();
        size_t srcIdx = 0;

        std::cout << "  Building internal level " << levelIdx
                  << " (" << numNodesInLevel << " nodes)" << std::endl;

        for (size_t nodeIdx = 0; nodeIdx < numNodesInLevel && srcIdx < currentLevelSize; ++nodeIdx) {
            const size_t nodeOffset = (levelStartIndex + nodeIdx) * MAX_INTERNAL_KEYS;
            const size_t remaining = currentLevelSize - srcIdx;

            if (remaining == 0) {
                std::cerr << "Warning: empty internal node at level " << levelIdx << std::endl;
                break;
            }

            size_t keysForNode = 0;

            if (remaining >= MAX_INTERNAL_CHILDREN) {
                keysForNode = MAX_INTERNAL_CHILDREN - 1;
                for (size_t keyIdx = 0; keyIdx < keysForNode; ++keyIdx) {
                    allInternalNodes[nodeOffset + keyIdx] = currentLevel[srcIdx + keyIdx];
                }
                nextLevel.push_back(currentLevel[srcIdx + keysForNode]);
                srcIdx += MAX_INTERNAL_CHILDREN;
            } else {
                keysForNode = (remaining > 0) ? remaining - 1 : 0;
                for (size_t keyIdx = 0; keyIdx < keysForNode; ++keyIdx) {
                    allInternalNodes[nodeOffset + keyIdx] = currentLevel[srcIdx + keyIdx];
                }
                if (remaining > 0) {
                    nextLevel.push_back(currentLevel[srcIdx + keysForNode]);
                }
                srcIdx = currentLevelSize;
            }
        }

        if (srcIdx != currentLevelSize) {
            std::cerr << "Warning: leftover maxima while building level " << levelIdx << std::endl;
        }

        currentLevel = std::move(nextLevel);
        nextWriteIndex = levelStartIndex;
    }
    
    // Step 5: Flush all internal nodes to disk at offset Page::PAGE_SIZE
    off_t diskOffset = Page::PAGE_SIZE;  // Page 0 is metadata, page 1+ are internal nodes
    size_t totalBytes = ctx.totalInternalNodes * Page::PAGE_SIZE;
    
    ssize_t written = pwrite(ctx.fd, allInternalNodes, totalBytes, diskOffset);
    
    if (written != static_cast<ssize_t>(totalBytes)) {
        std::cerr << "Error: Failed to write internal nodes to disk (wrote " 
                  << written << " bytes, expected " << totalBytes << ")" << std::endl;
    } else {
        std::cout << "  All " << ctx.totalInternalNodes 
                  << " internal nodes written to disk" << std::endl;
    }
    
    delete[] allInternalNodes;
}

bool BTreeSST::writeMetadata(BuildContext& ctx) {
    /*
     * Input:
     *   - ctx: Build context with the completed tree metadata
     * Algorithm:
     *   1. Populate a MetadataPage struct from ctx.
     *   2. Serialize it into a Page buffer.
     *   3. Write the buffer to disk at page 0.
     */
    MetadataPage metadataPage;
    metadataPage.minKey = ctx.minKey;
    metadataPage.maxKey = ctx.maxKey;
    metadataPage.treeHeight = ctx.treeHeight;
    metadataPage.leafCount = ctx.leafNodeCount;
    metadataPage.lastLeafPairs = ctx.lastLeafPairs;
    
    // Set level 0 (leaf level) metadata
    metadataPage.nodesPerLevel[0] = ctx.leafNodeCount;
    metadataPage.lastNodeKeys[0] = ctx.lastLeafPairs;
    
    // Copy internal level metadata if available (levels 1+)
    if (ctx.nodesPerLevel && ctx.lastNodeKeys) {
        for (size_t i = 0; i < ctx.internalLevelCount && (i + 1) < MAX_TREE_HEIGHT; i++) {
            metadataPage.nodesPerLevel[i + 1] = ctx.nodesPerLevel[i];
            metadataPage.lastNodeKeys[i + 1] = ctx.lastNodeKeys[i];
        }
    }

    Page page;
    if (!page.serialize(metadataPage)) {
        std::cerr << "Error: Failed to serialize metadata page" << std::endl;
        return false;
    }

    if (!writePage(ctx.fd, 0, page)) {
        std::cerr << "Error: Failed to write metadata page to disk" << std::endl;
        return false;
    }

    return true;
}


/**
 * Write a single page to disk at specified page ID
 */
bool BTreeSST::writePage(int fd, uint32_t pageId, const Page& page) const {
    off_t offset = static_cast<off_t>(pageId) * Page::PAGE_SIZE;
    ssize_t written = pwrite(fd, page.getData(), Page::PAGE_SIZE, offset);
    
    if (written != Page::PAGE_SIZE) {
        std::cerr << "Error: Failed to write page " << pageId << " (wrote " << written << " bytes)" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * Read metadata from SST file
 */
bool BTreeSST::readMetadata(const std::string& fileName, MetadataPage& metadata) const {
    Page page;
    if (!readPage(fileName, 0, page)) {
        return false;
    }
    return metadata.deserialize(page);
}

/**
 * Read a page from disk
 */
bool BTreeSST::readPage(const std::string& fileName, uint32_t pageId, Page& page) const {
    int fd = open(fileName.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Error: Cannot open file for reading: " << fileName << std::endl;
        return false;
    }
    
    off_t offset = static_cast<off_t>(pageId) * Page::PAGE_SIZE;
    ssize_t bytesRead = pread(fd, page.getData(), Page::PAGE_SIZE, offset);
    close(fd);
    
    if (bytesRead != Page::PAGE_SIZE) {
        std::cerr << "Error: Failed to read page " << pageId << " (read " << bytesRead << " bytes)" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * Point query - find value for a given key
 */
bool BTreeSST::get(int key, int& value, const std::string& fileName, bool useBTreeSearch) {
    // Read metadata first
    MetadataPage metadata;
    if (!readMetadata(fileName, metadata)) {
        std::cerr << "Error: Failed to read metadata from " << fileName << std::endl;
        return false;
    }
    
    // Quick filter using minKey/maxKey
    if (key < metadata.minKey || key > metadata.maxKey) {
        return false;
    }
    
    if (useBTreeSearch) {
        return getBTreeSearch(key, value, fileName, metadata);
    } else {
        return getBinarySearch(key, value, fileName, metadata);
    }
}

/**
 * B-Tree search: traverse from root to leaf
 */
bool BTreeSST::getBTreeSearch(int key, int& value, const std::string& fileName, const MetadataPage& metadata) {
    // TODO: Implement in next phase
    std::cerr << "B-Tree search not yet implemented" << std::endl;
    return false;
}

/**
 * Binary search: search directly on leaf pages (for comparison)
 */
bool BTreeSST::getBinarySearch(int key, int& value, const std::string& fileName, const MetadataPage& metadata) {
    // Simple linear scan through leaf pages for testing
    int fd = open(fileName.c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    }
    
    // Calculate total pairs
    uint32_t totalPairs = 0;
    if (metadata.leafCount > 0) {
        totalPairs = (metadata.leafCount - 1) * MAX_LEAF_PAIRS + metadata.lastLeafPairs;
    }
    
    // Calculate total internal nodes (if tree height > 1)
    uint32_t totalInternalNodes = 0;
    if (metadata.treeHeight > 1) {
        for (uint32_t i = 1; i < metadata.treeHeight; i++) {
            totalInternalNodes += metadata.nodesPerLevel[i];
        }
    }
    
    // Read all leaf data
    // Leaf pages start after metadata page (page 0) and internal nodes
    size_t leafDataSize = totalPairs * 2 * sizeof(int32_t);
    int32_t* leafData = new int32_t[totalPairs * 2];
    
    // Offset = metadata page + internal node pages
    size_t leafOffset = (1 + totalInternalNodes) * Page::PAGE_SIZE;
    ssize_t bytesRead = pread(fd, leafData, leafDataSize, leafOffset);
    close(fd);
    
    if (bytesRead != static_cast<ssize_t>(leafDataSize)) {
        delete[] leafData;
        return false;
    }
    
    // Linear search through interleaved key-value pairs
    for (uint32_t i = 0; i < totalPairs; i++) {
        int32_t currentKey = leafData[i * 2];
        if (currentKey == key) {
            value = leafData[i * 2 + 1];
            delete[] leafData;
            return true;
        }
    }
    
    delete[] leafData;
    return false;
}

/**
 * Range scan - find all key-value pairs in range [key1, key2]
 */
std::vector<std::pair<int, int>> BTreeSST::scan(int key1, int key2, const std::string& fileName, bool useBTreeSearch) {
    std::vector<std::pair<int, int>> results;
    
    // Read metadata first
    MetadataPage metadata;
    if (!readMetadata(fileName, metadata)) {
        return results;
    }
    
    // Quick filter using minKey/maxKey
    if (key2 < metadata.minKey || key1 > metadata.maxKey) {
        return results;
    }
    
    // Simple linear scan through leaf pages
    int fd = open(fileName.c_str(), O_RDONLY);
    if (fd < 0) {
        return results;
    }
    
    // Calculate total pairs
    uint32_t totalPairs = 0;
    if (metadata.leafCount > 0) {
        totalPairs = (metadata.leafCount - 1) * MAX_LEAF_PAIRS + metadata.lastLeafPairs;
    }
    
    // Calculate total internal nodes (if tree height > 1)
    uint32_t totalInternalNodes = 0;
    if (metadata.treeHeight > 1) {
        for (uint32_t i = 1; i < metadata.treeHeight; i++) {
            totalInternalNodes += metadata.nodesPerLevel[i];
        }
    }
    
    // Read all leaf data
    size_t leafDataSize = totalPairs * 2 * sizeof(int32_t);
    int32_t* leafData = new int32_t[totalPairs * 2];
    
    // Offset = metadata page + internal node pages
    size_t leafOffset = (1 + totalInternalNodes) * Page::PAGE_SIZE;
    ssize_t bytesRead = pread(fd, leafData, leafDataSize, leafOffset);
    close(fd);
    
    if (bytesRead != static_cast<ssize_t>(leafDataSize)) {
        delete[] leafData;
        return results;
    }
    
    // Scan through pairs and collect those in range
    for (uint32_t i = 0; i < totalPairs; i++) {
        int32_t currentKey = leafData[i * 2];
        int32_t currentValue = leafData[i * 2 + 1];
        
        if (currentKey >= key1 && currentKey <= key2) {
            results.push_back({currentKey, currentValue});
        }
    }
    
    delete[] leafData;
    return results;
}

/**
 * Convert SST to sorted array
 */
std::vector<std::pair<int, int>> BTreeSST::toSortedArray(const std::string& fileName) {
    // TODO: Implement in next phase
    std::cerr << "toSortedArray not yet implemented" << std::endl;
    return {};
}
