#include "LSMTree.hpp"
#include "../FileOperations.hpp"
#include "../BTree/BTreeNode.hpp"
#include "../BufferPool/Page.hpp"
#include "../DBConfig.hpp"
#include "MergeBuffer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <fcntl.h>
#include <unistd.h>

// Constructor
LSMTree::LSMTree(const std::string& directory, BufferPool* pool) 
    : dbDirectory(directory), sstBuilder(pool), nextSSTNumber(1) {
    
    // Initialize empty levels vector
    levels.clear();
    
    // Try to load existing manifest
    if (!loadManifest()) {
        VERBOSE_PRINT("No existing manifest found, starting with empty LSM tree");
    } else {
        VERBOSE_PRINT("Loaded LSM tree structure from manifest");
    }
}

// Destructor
LSMTree::~LSMTree() {
    // Save manifest before cleanup
    if (!levels.empty()) {
        saveManifest();
    }
    
    // Clear levels vector
    levels.clear();
}

// Manifest loading
bool LSMTree::loadManifest() {
    std::string manifestPath = dbDirectory + "/manifest.txt";
    
    // Check if manifest file exists
    if (!FileOperations::file_exists(manifestPath)) {
        // No manifest file - this is a new/empty database
        return false;
    }
    
    // Parse manifest file
    std::vector<SSTMetadata> allMetadata;
    if (!ManifestUtils::parseManifest(manifestPath, allMetadata, nextSSTNumber)) {
        std::cerr << "Error: Failed to parse manifest file" << std::endl;
        return false;
    }
    
    // Clear existing levels
    levels.clear();
    
    // Organize SSTs by level
    for (const auto& sst : allMetadata) {
        // Ensure we have enough levels
        while (levels.size() <= static_cast<size_t>(sst.level)) {
            levels.push_back(std::vector<SSTMetadata>());
        }
        
        // Add SST to its level
        levels[sst.level].push_back(sst);
    }
    
    VERBOSE_PRINT("Loaded " << allMetadata.size() << " SSTs from manifest");
    return true;
}

// Manifest saving
bool LSMTree::saveManifest() {
    std::string manifestPath = dbDirectory + "/manifest.txt";
    
    // Flatten all SSTs from all levels into a single vector
    std::vector<SSTMetadata> allMetadata;
    for (const auto& level : levels) {
        for (const auto& sst : level) {
            allMetadata.push_back(sst);
        }
    }
    
    // Write manifest atomically
    if (!ManifestUtils::writeManifest(manifestPath, allMetadata, nextSSTNumber)) {
        std::cerr << "Error: Failed to write manifest file" << std::endl;
        return false;
    }
    
    return true;
}

// Check if compaction is needed at a given level
bool LSMTree::needsCompaction(int level) const {
    // Check if level exists and has two or more SSTs
    if (level < 0 || level >= static_cast<int>(levels.size())) {
        return false;
    }
    
    return levels[level].size() >= 2;
}

// Compact a level by merging two SSTs into the next level
bool LSMTree::compactLevel(int level) {
    // Validate level exists and has at least 2 SSTs
    if (level < 0 || level >= static_cast<int>(levels.size())) {
        std::cerr << "Error: Invalid level " << level << " for compaction" << std::endl;
        return false;
    }
    
    if (levels[level].size() < 2) {
        std::cerr << "Error: Level " << level << " has fewer than 2 SSTs, cannot compact" << std::endl;
        return false;
    }
    
    // Select the first two SSTs from the level for merging
    // Note: sst2 is newer (added later), so it should be the first parameter to win on duplicates
    SSTMetadata sst1 = levels[level][1];  // Newer SST
    SSTMetadata sst2 = levels[level][0];  // Older SST
    
    VERBOSE_PRINT("Compacting Level " << level << ": merging " 
              << sst1.fileName << " (newer) and " << sst2.fileName << " (older)");
    
    // Generate output SST filename for target level (level + 1)
    int targetLevel = level + 1;
    int outputSSTNumber = nextSSTNumber;  // Save the number before incrementing
    std::string outputFileName = "sst_" + std::to_string(outputSSTNumber) + ".txt";
    std::string outputPath = dbDirectory + "/" + outputFileName;
    
    // Perform the merge
    if (!mergeTwoSSTs(sst1.fileName, sst2.fileName, outputFileName, targetLevel)) {
        std::cerr << "Error: Failed to merge SSTs during compaction" << std::endl;
        return false;
    }
    
    // Extract metadata from the newly created output SST
    std::string fullOutputPath = dbDirectory + "/" + outputFileName;
    std::ifstream file(fullOutputPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open merged SST file: " << fullOutputPath << std::endl;
        return false;
    }
    
    Page metaPage;
    file.read(metaPage.getData(), Page::PAGE_SIZE);
    file.close();
    
    MetadataPage metadata;
    if (!metadata.deserialize(metaPage)) {
        std::cerr << "Error: Invalid metadata in merged SST file" << std::endl;
        return false;
    }
    
    // Calculate total number of pairs
    uint32_t totalPairs = 0;
    if (metadata.leafCount > 0) {
        totalPairs = (metadata.leafCount - 1) * MAX_LEAF_PAIRS + metadata.lastLeafPairs;
    }
    
    // Create metadata for the output SST
    SSTMetadata outputMetadata(
        outputFileName,
        metadata.minKey,
        metadata.maxKey,
        totalPairs,
        targetLevel,
        outputSSTNumber
    );
    
    // Increment nextSSTNumber for the next SST
    nextSSTNumber++;
    
    // Update levels vector: remove source SSTs from current level
    // Remove both SSTs (indices 0 and 1)
    levels[level].erase(levels[level].begin());  // Remove first SST (was at index 0)
    levels[level].erase(levels[level].begin());  // Remove second SST (now at position 0)
    
    // Ensure target level exists
    while (levels.size() <= static_cast<size_t>(targetLevel)) {
        levels.push_back(std::vector<SSTMetadata>());
    }
    
    // Add output SST to target level
    levels[targetLevel].push_back(outputMetadata);
    
    VERBOSE_PRINT("Added merged SST " << outputFileName << " to Level " << targetLevel);
    
    // Delete source SST files from disk
    std::string fullPath1 = dbDirectory + "/" + sst1.fileName;
    std::string fullPath2 = dbDirectory + "/" + sst2.fileName;
    
    if (!FileOperations::remove_file(fullPath1)) {
        std::cerr << "Warning: Failed to delete source SST file: " << fullPath1 << std::endl;
    }
    if (!FileOperations::remove_file(fullPath2)) {
        std::cerr << "Warning: Failed to delete source SST file: " << fullPath2 << std::endl;
    }
    
    // Save updated manifest
    if (!saveManifest()) {
        std::cerr << "Error: Failed to save manifest after compaction" << std::endl;
        return false;
    }
    
    VERBOSE_PRINT("Compaction of Level " << level << " completed successfully");
    
    // Check if target level now needs compaction (cascade)
    if (needsCompaction(targetLevel)) {
        VERBOSE_PRINT("Cascade compaction needed at Level " << targetLevel);
        return compactLevel(targetLevel);
    }
    
    return true;
}

// Merge two SSTs using streaming algorithm with fixed-size buffers
bool LSMTree::mergeTwoSSTs(const std::string& sst1, const std::string& sst2,
                           const std::string& outputSST, int targetLevel) {
    VERBOSE_PRINT("Merging " << sst1 << " and " << sst2 << " into " << outputSST);
    
    // Build full paths
    std::string fullPath1 = dbDirectory + "/" + sst1;
    std::string fullPath2 = dbDirectory + "/" + sst2;
    std::string fullOutputPath = dbDirectory + "/" + outputSST;
    
    // Verify input files exist
    if (!FileOperations::file_exists(fullPath1)) {
        std::cerr << "Error: Input SST file does not exist: " << fullPath1 << std::endl;
        return false;
    }
    if (!FileOperations::file_exists(fullPath2)) {
        std::cerr << "Error: Input SST file does not exist: " << fullPath2 << std::endl;
        return false;
    }
    
    // Read metadata from both input SSTs to determine where leaf data starts
    std::ifstream file1(fullPath1, std::ios::binary);
    std::ifstream file2(fullPath2, std::ios::binary);
    
    if (!file1.is_open() || !file2.is_open()) {
        std::cerr << "Error: Failed to open input SST files" << std::endl;
        return false;
    }
    
    // Read metadata pages
    Page metaPage1, metaPage2;
    file1.read(metaPage1.getData(), Page::PAGE_SIZE);
    file2.read(metaPage2.getData(), Page::PAGE_SIZE);
    file1.close();
    file2.close();
    
    MetadataPage meta1, meta2;
    if (!meta1.deserialize(metaPage1) || !meta2.deserialize(metaPage2)) {
        std::cerr << "Error: Invalid metadata in input SST files" << std::endl;
        return false;
    }
    
    // Calculate starting offsets for leaf data using leaf_start_page from metadata
    // This properly skips metadata page (page 0) and all internal nodes
    size_t offset1 = meta1.leaf_start_page * Page::PAGE_SIZE;
    size_t offset2 = meta2.leaf_start_page * Page::PAGE_SIZE;
    
    // Calculate end of leaf data (exact byte offset where key-value pairs end)
    // All full leaf pages + partial last leaf page
    // Each pair is 2 * sizeof(int32_t) = 8 bytes
    size_t leafDataBytes1 = meta1.total_number_of_pairs * 2 * sizeof(int32_t);
    size_t leafDataBytes2 = meta2.total_number_of_pairs * 2 * sizeof(int32_t);
    size_t maxOffset1 = offset1 + leafDataBytes1;
    size_t maxOffset2 = offset2 + leafDataBytes2;
    
    VERBOSE_PRINT("SST1: leaf_start_page=" << meta1.leaf_start_page 
              << ", leafCount=" << meta1.leafCount 
              << ", total_pairs=" << meta1.total_number_of_pairs);
    VERBOSE_PRINT("SST2: leaf_start_page=" << meta2.leaf_start_page 
              << ", leafCount=" << meta2.leafCount 
              << ", total_pairs=" << meta2.total_number_of_pairs);
    
    // Allocate merge buffers (2 input + 1 output) with BufferPool support
    MergeBuffer inputBuffer1(MergeBuffer::DEFAULT_BUFFER_SIZE, sstBuilder.getBufferPool());
    MergeBuffer inputBuffer2(MergeBuffer::DEFAULT_BUFFER_SIZE, sstBuilder.getBufferPool());
    MergeBuffer outputBuffer(MergeBuffer::DEFAULT_BUFFER_SIZE, sstBuilder.getBufferPool());
    
    // Initial refill of input buffers
    bool hasData1 = inputBuffer1.refillFromSST(fullPath1, offset1, maxOffset1);
    bool hasData2 = inputBuffer2.refillFromSST(fullPath2, offset2, maxOffset2);
    
    if (!hasData1 && !hasData2) {
        std::cerr << "Error: Both input SSTs are empty" << std::endl;
        return false;
    }
    
    // Create temporary file for streaming merged data
    std::string tempMergePath = fullOutputPath + ".merge.tmp";
    int mergeFd = open(tempMergePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (mergeFd < 0) {
        std::cerr << "Error: Failed to create temporary merge file: " << tempMergePath << std::endl;
        return false;
    }
    
    // Track total pairs merged
    size_t totalMergedPairs = 0;
    
    // Helper lambda to add pair to output buffer and flush to file if full
    auto addToOutput = [&](int32_t key, int32_t value) {
        if (outputBuffer.isFull()) {
            // Flush output buffer to file
            if (!outputBuffer.flushToFile(mergeFd)) {
                std::cerr << "Error: Failed to flush output buffer to file" << std::endl;
                close(mergeFd);
                FileOperations::remove_file(tempMergePath);
                return false;
            }
            outputBuffer.clear();
        }
        outputBuffer.append(key, value);
        totalMergedPairs++;
        return true;
    };
    
    // Main merge loop: process data from both buffers
    while (hasData1 && hasData2) {
        // Compare minimum keys from both buffers
        auto pair1 = inputBuffer1.peekMin();
        auto pair2 = inputBuffer2.peekMin();
        
        if (pair1.first < pair2.first) {
            // Key from buffer1 is smaller - take it
            if (!addToOutput(pair1.first, pair1.second)) {
                close(mergeFd);
                return false;
            }
            inputBuffer1.consumeMin();
            
            // Refill buffer1 if exhausted
            if (!inputBuffer1.hasData()) {
                hasData1 = inputBuffer1.refillFromSST(fullPath1, offset1, maxOffset1);
            }
        } else if (pair1.first > pair2.first) {
            // Key from buffer2 is smaller - take it
            if (!addToOutput(pair2.first, pair2.second)) {
                close(mergeFd);
                return false;
            }
            inputBuffer2.consumeMin();
            
            // Refill buffer2 if exhausted
            if (!inputBuffer2.hasData()) {
                hasData2 = inputBuffer2.refillFromSST(fullPath2, offset2, maxOffset2);
            }
        } else {
            // Keys are equal - keep pair from sst1 (newer) and discard sst2's version
            if (!addToOutput(pair1.first, pair1.second)) {
                close(mergeFd);
                return false;
            }
            inputBuffer1.consumeMin();
            inputBuffer2.consumeMin();
            
            // Refill both buffers if exhausted
            if (!inputBuffer1.hasData()) {
                hasData1 = inputBuffer1.refillFromSST(fullPath1, offset1, maxOffset1);
            }
            if (!inputBuffer2.hasData()) {
                hasData2 = inputBuffer2.refillFromSST(fullPath2, offset2, maxOffset2);
            }
        }
    }
    
    // Handle remaining data from buffer1
    while (hasData1) {
        auto pair = inputBuffer1.peekMin();
        if (!addToOutput(pair.first, pair.second)) {
            close(mergeFd);
            return false;
        }
        inputBuffer1.consumeMin();
        
        if (!inputBuffer1.hasData()) {
            hasData1 = inputBuffer1.refillFromSST(fullPath1, offset1, maxOffset1);
        }
    }
    
    // Handle remaining data from buffer2
    while (hasData2) {
        auto pair = inputBuffer2.peekMin();
        if (!addToOutput(pair.first, pair.second)) {
            close(mergeFd);
            return false;
        }
        inputBuffer2.consumeMin();
        
        if (!inputBuffer2.hasData()) {
            hasData2 = inputBuffer2.refillFromSST(fullPath2, offset2, maxOffset2);
        }
    }
    
    // Flush any remaining data in output buffer
    if (outputBuffer.validPairs > 0) {
        if (!outputBuffer.flushToFile(mergeFd)) {
            std::cerr << "Error: Failed to flush final output buffer" << std::endl;
            close(mergeFd);
            FileOperations::remove_file(tempMergePath);
            return false;
        }
    }
    
    // Close merge file
    close(mergeFd);
    
    VERBOSE_PRINT("Merged " << totalMergedPairs << " pairs to temporary file");
    
    // Now read the merged data back and build B-Tree SST
    // Read merged data from temporary file
    std::vector<std::pair<int, int>> mergedData;
    mergedData.reserve(totalMergedPairs);
    
    int readFd = open(tempMergePath.c_str(), O_RDONLY);
    if (readFd < 0) {
        std::cerr << "Error: Failed to open temporary merge file for reading" << std::endl;
        FileOperations::remove_file(tempMergePath);
        return false;
    }
    
    // Read all merged data
    size_t bytesToRead = totalMergedPairs * 2 * sizeof(int32_t);
    int32_t* readBuffer = new int32_t[totalMergedPairs * 2];
    ssize_t bytesRead = read(readFd, readBuffer, bytesToRead);
    close(readFd);
    
    if (bytesRead != static_cast<ssize_t>(bytesToRead)) {
        std::cerr << "Error: Failed to read merged data from temporary file" << std::endl;
        delete[] readBuffer;
        FileOperations::remove_file(tempMergePath);
        return false;
    }
    
    // Convert to vector of pairs
    for (size_t i = 0; i < totalMergedPairs; i++) {
        mergedData.emplace_back(readBuffer[i * 2], readBuffer[i * 2 + 1]);
    }
    delete[] readBuffer;
    
    // Delete temporary merge file
    FileOperations::remove_file(tempMergePath);
    
    // Build final B-Tree SST from merged data
    std::string tempOutputPath = fullOutputPath + ".tmp";
    if (!sstBuilder.buildBTree(mergedData, tempOutputPath)) {
        std::cerr << "Error: Failed to build B-Tree from merged data" << std::endl;
        FileOperations::remove_file(tempOutputPath);
        return false;
    }
    
    // Atomically rename temporary file to final output
    if (rename(tempOutputPath.c_str(), fullOutputPath.c_str()) != 0) {
        std::cerr << "Error: Failed to rename temporary file to final output" << std::endl;
        FileOperations::remove_file(tempOutputPath);
        return false;
    }
    
    VERBOSE_PRINT("Successfully merged " << totalMergedPairs << " pairs into " << outputSST);
    return true;
}

// Add SST to LSM tree
bool LSMTree::addSST(const std::string& sstFile, int level) {
    // Extract metadata from SST file by reading MetadataPage
    std::string fullPath = dbDirectory + "/" + sstFile;
    
    // Check if file exists
    if (!FileOperations::file_exists(fullPath)) {
        std::cerr << "Error: SST file does not exist: " << fullPath << std::endl;
        return false;
    }
    
    // Read metadata from SST file using BTreeSST
    // We need to read page 0 which contains the MetadataPage
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open SST file: " << fullPath << std::endl;
        return false;
    }
    
    // Read the metadata page (page 0)
    Page metaPage;
    file.read(metaPage.getData(), Page::PAGE_SIZE);
    if (!file.good()) {
        std::cerr << "Error: Failed to read metadata from SST file: " << fullPath << std::endl;
        file.close();
        return false;
    }
    file.close();
    
    // Deserialize metadata
    MetadataPage metadata;
    if (!metadata.deserialize(metaPage)) {
        std::cerr << "Error: Invalid metadata in SST file: " << fullPath << std::endl;
        return false;
    }
    
    // Calculate total number of pairs
    uint32_t totalPairs = 0;
    if (metadata.leafCount > 0) {
        // All full leaves + last leaf
        totalPairs = (metadata.leafCount - 1) * MAX_LEAF_PAIRS + metadata.lastLeafPairs;
    }
    
    // Create SSTMetadata object
    SSTMetadata sstMetadata(
        sstFile,
        metadata.minKey,
        metadata.maxKey,
        totalPairs,
        level,
        nextSSTNumber++
    );
    
    // Ensure we have enough levels
    while (levels.size() <= static_cast<size_t>(level)) {
        levels.push_back(std::vector<SSTMetadata>());
    }
    
    // Add to appropriate level
    levels[level].push_back(sstMetadata);
    
    VERBOSE_PRINT("Added SST " << sstFile << " to Level " << level 
              << " [" << metadata.minKey << ", " << metadata.maxKey << "]"
              << " (" << totalPairs << " pairs)");
    
    // Save updated manifest
    if (!saveManifest()) {
        std::cerr << "Warning: Failed to save manifest after adding SST" << std::endl;
    }
    
    // Check if compaction is needed after adding SST
    if (needsCompaction(level)) {
        VERBOSE_PRINT("Compaction needed at Level " << level);
        // Trigger compaction for this level
        if (!compactLevel(level)) {
            std::cerr << "Warning: Compaction failed at Level " << level << std::endl;
            // Don't fail the addSST operation - the SST was successfully added
            // Compaction can be retried later
        }
        // Note: compactLevel handles cascade compaction internally
    }
    
    return true;
}

// Point query - search levels in ascending order
bool LSMTree::get(int key, int& value, bool useBTreeSearch) {
    // Search levels in ascending order (0, 1, 2, ...)
    // Level 0 contains the most recent data
    for (size_t levelIdx = 0; levelIdx < levels.size(); levelIdx++) {
        const auto& level = levels[levelIdx];
        
        // Iterate through SSTs in this level
        for (const auto& sst : level) {
            // Check if key is in range using mightContain
            if (!sst.mightContain(key)) {
                continue;  // Skip this SST - key not in range
            }
            
            // Key might be in this SST - query it using BTreeSST::get with specified search mode
            std::string fullPath = dbDirectory + "/" + sst.fileName;
            if (sstBuilder.get(key, value, fullPath, useBTreeSearch)) {
                // Found the key - return immediately (most recent version)
                return true;
            }
            // Key not found in this SST, continue searching
        }
    }
    
    // Key not found in any level
    return false;
}

// Range scan - collect and deduplicate results from all levels
std::vector<std::pair<int, int>> LSMTree::scan(int key1, int key2) {
    // Use a map to handle duplicates - keeps the first (most recent) version of each key
    // Map automatically maintains sorted order by key
    std::map<int, int> resultMap;
    
    // Collect matching entries from all levels
    for (size_t levelIdx = 0; levelIdx < levels.size(); levelIdx++) {
        const auto& level = levels[levelIdx];
        
        // Iterate through SSTs in this level
        for (const auto& sst : level) {
            // Check if SST's key range overlaps with query range
            // Skip if SST's max key is less than key1 or min key is greater than key2
            if (sst.maxKey < key1 || sst.minKey > key2) {
                continue;  // No overlap - skip this SST
            }
            
            // Query this SST using BTreeSST::scan (use B-Tree search)
            std::string fullPath = dbDirectory + "/" + sst.fileName;
            std::vector<std::pair<int, int>> sstResults = sstBuilder.scan(key1, key2, fullPath);  // Use B-Tree search (default)
            
            // Add results to map - only insert if key doesn't already exist
            // This ensures we keep the version from the lowest level (most recent)
            for (const auto& pair : sstResults) {
                // insert only adds if key doesn't exist
                resultMap.insert(pair);
            }
        }
    }
    
    // Convert map to vector (already in sorted key order)
    std::vector<std::pair<int, int>> results;
    results.reserve(resultMap.size());
    for (const auto& pair : resultMap) {
        results.push_back(pair);
    }
    
    return results;
}

// Get next SST number
int LSMTree::getNextSSTNumber() {
    return nextSSTNumber++;
}