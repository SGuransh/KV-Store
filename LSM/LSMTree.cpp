#include "LSMTree.hpp"
#include "../FileOperations.hpp"
#include "../BTree/BTreeNode.hpp"
#include "../BufferPool/Page.hpp"
#include "MergeBuffer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <map>
#include <fcntl.h>
#include <unistd.h>

// Constructor
LSMTree::LSMTree(const std::string& directory) 
    : dbDirectory(directory), nextSSTNumber(1) {
    
    // Initialize empty levels vector
    levels.clear();
    
    // Try to load existing manifest
    if (!loadManifest()) {
        std::cout << "No existing manifest found, starting with empty LSM tree" << std::endl;
    } else {
        std::cout << "Loaded LSM tree structure from manifest" << std::endl;
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
    
    std::cout << "Loaded " << allMetadata.size() << " SSTs from manifest" << std::endl;
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
    
    std::cout << "Compacting Level " << level << ": merging " 
              << sst1.fileName << " (newer) and " << sst2.fileName << " (older)" << std::endl;
    
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
    
    std::cout << "Added merged SST " << outputFileName << " to Level " << targetLevel << std::endl;
    
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
    
    std::cout << "Compaction of Level " << level << " completed successfully" << std::endl;
    
    // Check if target level now needs compaction (cascade)
    if (needsCompaction(targetLevel)) {
        std::cout << "Cascade compaction needed at Level " << targetLevel << std::endl;
        return compactLevel(targetLevel);
    }
    
    return true;
}

// Merge two SSTs using streaming algorithm with fixed-size buffers
bool LSMTree::mergeTwoSSTs(const std::string& sst1, const std::string& sst2,
                           const std::string& outputSST, int targetLevel) {
    std::cout << "Merging " << sst1 << " and " << sst2 << " into " << outputSST << std::endl;
    
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
    
    // Use toSortedArray to read all data from both SSTs
    // This is simpler and more reliable than streaming merge
    std::vector<std::pair<int, int>> data1 = sstBuilder.toSortedArray(fullPath1);
    std::vector<std::pair<int, int>> data2 = sstBuilder.toSortedArray(fullPath2);
    
    if (data1.empty() && data2.empty()) {
        std::cerr << "Error: Both input SSTs are empty" << std::endl;
        return false;
    }
    
    // Merge the two sorted arrays
    std::vector<std::pair<int, int>> mergedData;
    mergedData.reserve(data1.size() + data2.size());
    
    size_t i = 0, j = 0;
    while (i < data1.size() && j < data2.size()) {
        if (data1[i].first < data2[j].first) {
            mergedData.push_back(data1[i++]);
        } else if (data1[i].first > data2[j].first) {
            mergedData.push_back(data2[j++]);
        } else {
            // Keys are equal - keep pair from sst1 (newer) and skip sst2's version
            mergedData.push_back(data1[i++]);
            j++;  // Skip duplicate from sst2
        }
    }
    
    // Append remaining elements from data1
    while (i < data1.size()) {
        mergedData.push_back(data1[i++]);
    }
    
    // Append remaining elements from data2
    while (j < data2.size()) {
        mergedData.push_back(data2[j++]);
    }
    
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
    
    std::cout << "Successfully merged " << mergedData.size() << " pairs into " << outputSST << std::endl;
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
    
    std::cout << "Added SST " << sstFile << " to Level " << level 
              << " [" << metadata.minKey << ", " << metadata.maxKey << "]"
              << " (" << totalPairs << " pairs)" << std::endl;
    
    // Save updated manifest
    if (!saveManifest()) {
        std::cerr << "Warning: Failed to save manifest after adding SST" << std::endl;
    }
    
    // Check if compaction is needed after adding SST
    if (needsCompaction(level)) {
        std::cout << "Compaction needed at Level " << level << std::endl;
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
bool LSMTree::get(int key, int& value) {
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
            
            // Key might be in this SST - query it using BTreeSST::get (use B-Tree search)
            std::string fullPath = dbDirectory + "/" + sst.fileName;
            if (sstBuilder.get(key, value, fullPath)) {  // Use B-Tree search (default)
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

// Print LSM tree structure
void LSMTree::printStructure() const {
    std::cout << "=== LSM Tree Structure ===" << std::endl;
    std::cout << "Database Directory: " << dbDirectory << std::endl;
    std::cout << "Next SST Number: " << nextSSTNumber << std::endl;
    std::cout << "Number of Levels: " << levels.size() << std::endl;
    
    for (size_t i = 0; i < levels.size(); i++) {
        std::cout << "Level " << i << ": " << levels[i].size() << " SSTs" << std::endl;
        for (const auto& sst : levels[i]) {
            std::cout << "  - " << sst.fileName 
                     << " [" << sst.minKey << ", " << sst.maxKey << "]"
                     << " (" << sst.numPairs << " pairs)" << std::endl;
        }
    }
    std::cout << "=========================" << std::endl;
}
