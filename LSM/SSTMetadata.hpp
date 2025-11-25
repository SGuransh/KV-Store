#pragma once

#ifndef SSTMETADATA_HPP
#define SSTMETADATA_HPP

#include <string>
#include <cstdint>
#include <vector>
#include <fstream>
#include <sstream>

/**
 * SSTMetadata - Stores metadata about an SST file for efficient querying
 * Used by LSMTree to track SST files across different levels
 */
struct SSTMetadata {
    std::string fileName;    // Name of the SST file (e.g., "sst_1.txt")
    int32_t minKey;          // Smallest key in this SST
    int32_t maxKey;          // Largest key in this SST
    uint32_t numPairs;       // Total number of key-value pairs
    int level;               // Level in LSM tree (0 = newest)
    int sstNumber;           // Unique identifier for this SST

    SSTMetadata() 
        : fileName(""), minKey(0), maxKey(0), numPairs(0), level(0), sstNumber(0) {}

    SSTMetadata(const std::string& file, int32_t min, int32_t max,
                uint32_t pairs, int lvl, int num)
        : fileName(file), minKey(min), maxKey(max), 
          numPairs(pairs), level(lvl), sstNumber(num) {}

    /**
     * Check if a key might be in this SST based on key range
     * @param key The key to check
     * @return true if key is in range [minKey, maxKey], false otherwise
     */
    bool mightContain(int key) const {
        return key >= minKey && key <= maxKey;
    }

    /**
     * Serialize this metadata to a manifest file line
     * Format: level,sstNumber,fileName,minKey,maxKey,numPairs
     * @return String representation for manifest file
     */
    std::string serialize() const {
        std::ostringstream oss;
        oss << level << "," << sstNumber << "," << fileName << "," 
            << minKey << "," << maxKey << "," << numPairs;
        return oss.str();
    }

    /**
     * Deserialize metadata from a manifest file line
     * Format: level,sstNumber,fileName,minKey,maxKey,numPairs
     * @param line The line to parse
     * @return true if parsing succeeded, false otherwise
     */
    bool deserialize(const std::string& line) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;

        // Split by comma
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }

        // Validate format
        if (tokens.size() != 6) {
            return false;
        }

        try {
            level = std::stoi(tokens[0]);
            sstNumber = std::stoi(tokens[1]);
            fileName = tokens[2];
            minKey = std::stoi(tokens[3]);
            maxKey = std::stoi(tokens[4]);
            numPairs = std::stoul(tokens[5]);
            return true;
        } catch (...) {
            return false;
        }
    }
};

/**
 * ManifestUtils - Helper functions for reading and writing manifest files
 * The manifest file tracks all SST files and their metadata for LSM tree recovery
 */
namespace ManifestUtils {
    /**
     * Parse the manifest file and return all SST metadata entries
     * @param manifestPath Path to the manifest file
     * @param metadata Output vector of SST metadata
     * @param nextSSTNumber Output parameter for the next SST number to use
     * @return true if successful, false if file doesn't exist or parse error
     */
    inline bool parseManifest(const std::string& manifestPath, 
                             std::vector<SSTMetadata>& metadata,
                             int& nextSSTNumber) {
        std::ifstream file(manifestPath);
        if (!file.is_open()) {
            return false;
        }

        metadata.clear();
        nextSSTNumber = 1;
        std::string line;

        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Check for next_sst_number line
            if (line.find("next_sst_number=") == 0) {
                try {
                    nextSSTNumber = std::stoi(line.substr(16));
                } catch (...) {
                    // Invalid format, keep default
                }
                continue;
            }

            // Parse SST metadata line
            SSTMetadata sst;
            if (sst.deserialize(line)) {
                metadata.push_back(sst);
            }
            // Silently skip invalid lines
        }

        file.close();
        return true;
    }

    /**
     * Write manifest file atomically using temporary file and rename
     * @param manifestPath Path to the manifest file
     * @param metadata Vector of all SST metadata to write
     * @param nextSSTNumber The next SST number to use
     * @return true if successful, false otherwise
     */
    inline bool writeManifest(const std::string& manifestPath,
                             const std::vector<SSTMetadata>& metadata,
                             int nextSSTNumber) {
        // Write to temporary file first
        std::string tempPath = manifestPath + ".tmp";
        std::ofstream file(tempPath);
        if (!file.is_open()) {
            return false;
        }

        // Write header
        file << "# LSM Tree Manifest\n";
        file << "# Format: level,sstNumber,fileName,minKey,maxKey,numPairs\n";
        file << "next_sst_number=" << nextSSTNumber << "\n";

        // Write all SST metadata entries
        for (const auto& sst : metadata) {
            file << sst.serialize() << "\n";
        }

        file.close();

        // Atomic rename (overwrites existing manifest)
        if (std::rename(tempPath.c_str(), manifestPath.c_str()) != 0) {
            // Rename failed, try to clean up temp file
            std::remove(tempPath.c_str());
            return false;
        }

        return true;
    }

    /**
     * Serialize a single SST metadata entry to string
     * @param sst The SST metadata to serialize
     * @return String representation
     */
    inline std::string serializeEntry(const SSTMetadata& sst) {
        return sst.serialize();
    }

    /**
     * Deserialize a single SST metadata entry from string
     * @param line The line to parse
     * @param sst Output parameter for the parsed metadata
     * @return true if successful, false otherwise
     */
    inline bool deserializeEntry(const std::string& line, SSTMetadata& sst) {
        return sst.deserialize(line);
    }
}

#endif // SSTMETADATA_HPP
