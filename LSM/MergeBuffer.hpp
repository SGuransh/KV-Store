#pragma once

#ifndef MERGEBUFFER_HPP
#define MERGEBUFFER_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <utility>

/**
 * MergeBuffer - A fixed-size buffer for streaming merge operations during compaction
 * 
 * This class provides buffered I/O for reading from SST files and writing merged data
 * to output files without loading entire SSTs into memory. It supports:
 * 
 * 1. Input buffer operations: refill from SST, peek/consume minimum key-value pairs
 * 2. Output buffer operations: append pairs, flush to file when full
 * 
 * The buffer stores key-value pairs in interleaved format: [k1,v1, k2,v2, ...]
 */
class MergeBuffer {
public:
    static constexpr size_t DEFAULT_BUFFER_SIZE = 1024;  // Number of key-value pairs
    
    int32_t* buffer;      // Interleaved array [k1,v1, k2,v2, ...]
    size_t capacity;      // Maximum number of key-value pairs
    size_t validPairs;    // Number of valid pairs in buffer

private:
    size_t currentPos;    // Current read position (for input buffers)

public:
    /**
     * Constructor - allocate buffer with configurable size
     * @param bufferSize Number of key-value pairs to buffer (default 1024)
     */
    explicit MergeBuffer(size_t bufferSize = DEFAULT_BUFFER_SIZE);
    
    /**
     * Destructor - deallocate buffer
     */
    ~MergeBuffer();
    
    // Disable copy constructor and assignment operator
    MergeBuffer(const MergeBuffer&) = delete;
    MergeBuffer& operator=(const MergeBuffer&) = delete;
    
    // === Input Buffer Operations ===
    
    /**
     * Refill buffer from SST file starting at given file offset
     * Reads the next chunk of data from the SST file using pread
     * @param sstFile Path to the SST file
     * @param fileOffset Current byte offset in the file (updated after read)
     * @param maxOffset Maximum byte offset to read (end of leaf data, before bloom filter)
     * @return true if data was read, false if end of file or error
     */
    bool refillFromSST(const std::string& sstFile, size_t& fileOffset, size_t maxOffset);
    
    /**
     * Check if buffer contains unprocessed pairs
     * @return true if there are pairs available to read
     */
    bool hasData() const;
    
    /**
     * Peek at the minimum (next) key-value pair without consuming it
     * @return The next key-value pair
     */
    std::pair<int32_t, int32_t> peekMin() const;
    
    /**
     * Consume the minimum pair by advancing the read position
     */
    void consumeMin();
    
    /**
     * Get the number of valid pairs currently in the buffer
     * @return Number of valid pairs
     */
    size_t getValidPairs() const { return validPairs; }
    
    // === Output Buffer Operations ===
    
    /**
     * Append a key-value pair to the output buffer
     * @param key The key to append
     * @param value The value to append
     * @return true if successful, false if buffer is full
     */
    bool append(int32_t key, int32_t value);
    
    /**
     * Check if buffer has reached capacity
     * @return true if buffer is full
     */
    bool isFull() const;
    
    /**
     * Flush buffer contents to file descriptor
     * @param fd File descriptor to write to
     * @return true if successful, false on error
     */
    bool flushToFile(int fd);
    
    /**
     * Clear buffer and reset for next batch
     */
    void clear();
};

#endif // MERGEBUFFER_HPP
