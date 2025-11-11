#include "BTreeSST.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>

/**
 * Main entry point: Build B-Tree SST from sorted data and write to disk
 */
    std::cout << "Building internal levels bottom-up..." << std::endl;

    // Step 1: Maintain a dynamic array of max-values to be processed
    std::vector<int32_t> currentLevel(max_per_node, max_per_node + ctx.leafNodeCount);BTreeSST::buildBTree(const std::vector<std::pair<int, int>>& sortedData, const std::string& fileName) {
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
    
    // Phase 3: Build internal levels bottom-up (only if we have > 1 leaf)
    if (ctx.leafNodeCount > 1) {
        buildInternalLevels(ctx, max_per_node);
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
    """
    Input: 
        - max_per_node: Array of maximum keys in each leaf node
    Algorithm:
        1. Maintain a dynamic array of max-values to be processed, and the corresponding number of internal nodes for that level.
        2. Extract every Bth value in max array and keep it for future level. Now, for the current internal level,
            keep the first B-1 values and ignore the Bth value.
            For the last internal node, if there are X values, then keep X - 1 (possibly 0).
        3. Make an array of internal nodes stored contiguously of size (ctx.totalInternalNodes)
        4. for i in range(ctx.internalLevelCount) reverse iteration [2, 1, 0]:
            - Compute the internal nodes for level i using the max values from the previous level
            - Compute offset as sum(ctx.internalLevelSizes[0:i) exclusive) store it there
        5. Flush it to the file with the offset Page::Size, use pwrite and ctx.fd
    """
    """
        DRY RUN: B = 4
        - max_per_node = [3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60]

        3, 6, 9, *(12), 15, 18, 21, *(24), 27, 30, 33, *(36), 39, 42, 45, *(48), 51, 54, 57, *(60)

        (3,6,9) (15, 18, 21) (27,30,33) (39,42,45) (51,54,57)

        12, 24, 36, 48, 60


        max_per_node = [12, 24, 36, 48, 60]

        12, 24, 36, *(48), 60

        (12, 24, 36) ()

        48
        
        max_per_node = [48]
    """
    std::cout << "Building internal levels bottom-up..." << std::endl;

    // Step 1: Maintain a dynamic array of max-values to be processed
    std::vector<int32_t> currentLevel(max_per_node, max_per_node + ctx.leafNodeCount);
    // current_level = std::vector<int32_t>(max_per_node, max_per_node + ctx.leafNodeCount);
    
    // Step 3: Allocate contiguous array for all internal nodes
    int32_t* allInternalNodes = new int32_t[ctx.totalInternalNodes * MAX_INTERNAL_KEYS];
    std::memset(allInternalNodes, 0, ctx.totalInternalNodes * MAX_INTERNAL_KEYS * sizeof(int32_t));
    
    // Step 4: Build each internal level bottom-up (reverse iteration)
    //! Change this offset by Page::PAGE_SIZE after writing the keys for each node
    size_t writeOffset = 0;  // Offset in allInternalNodes array

    for (size_t levelIdx = ctx.internalLevelCount - 1; levelIdx >= 0; levelIdx--) {
        //! Reverse iteration
        size_t numNodesInLevel = ctx.internalLevelSizes[levelIdx];   // 5
        std::vector<int32_t> nextLevel;
        
        std::cout << "  Building internal level " << levelIdx 
                  << " (" << numNodesInLevel << " nodes)" << std::endl;
        
        // Step 2: Extract every Bth value for next level, keep B-1 values for current level
        size_t currentLevelSize = currentLevel.size();   
        size_t srcIdx = 0;
        
        for (size_t nodeIdx = 0; nodeIdx < numNodesInLevel; nodeIdx++) {
            size_t keysInThisNode = MAX_INTERNAL_KEYS;
            
            // Check if this is the last node in this level
            if (nodeIdx == numNodesInLevel - 1) {
                // Last node - might have fewer keys
                // Already have this value in ctx.lastNodeKeys
                size_t remainingChildren = currentLevelSize - srcIdx;
                if (remainingChildren > 0) {
                    keysInThisNode = remainingChildren - 1;  // X - 1 keys for X children
                } else {
                    keysInThisNode = 0;
                }
            }
            
            // Copy keys for this internal node
            for (size_t keyIdx = 0; keyIdx < keysInThisNode; keyIdx++) {
                if (srcIdx < currentLevelSize) {
                    allInternalNodes[writeOffset * MAX_INTERNAL_KEYS + keyIdx] = currentLevel[srcIdx];
                    srcIdx++;
                }
            }
            
            // Extract the Bth value (largest key) for next level
            if (srcIdx < currentLevelSize) {
                nextLevel.push_back(currentLevel[srcIdx]);
                srcIdx++;
            }
            
            writeOffset++;
        }
        
        // Move to next level
        currentLevel = nextLevel;
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
    """
    Input:
        - ctx: Build context with complete tree
    Algorithm:
        1. Make a MetadataPage object and populate its fields from ctx
        2. Serialize it to a Page object
        3. Write the Page to disk at page ID 0
    """
    MetadataPage metadataPage;
    metadataPage.minKey = ctx.minKey;
    metadataPage.maxKey = ctx.maxKey;
    metadataPage.treeHeight = ctx.treeHeight;
    metadataPage.internalLevelCount = ctx.internalLevelCount;
    metadataPage.totalInternalNodes = ctx.totalInternalNodes;

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
    // TODO: Implement in next phase
    std::cerr << "Binary search not yet implemented" << std::endl;
    return false;
}

/**
 * Range scan - find all key-value pairs in range [key1, key2]
 */
std::vector<std::pair<int, int>> BTreeSST::scan(int key1, int key2, const std::string& fileName, bool useBTreeSearch) {
    // TODO: Implement in next phase
    std::cerr << "Scan not yet implemented" << std::endl;
    return {};
}

/**
 * Convert SST to sorted array
 */
std::vector<std::pair<int, int>> BTreeSST::toSortedArray(const std::string& fileName) {
    // TODO: Implement in next phase
    std::cerr << "toSortedArray not yet implemented" << std::endl;
    return {};
}
