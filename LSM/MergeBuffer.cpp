#include "MergeBuffer.hpp"
#include "../BTree/BTreeNode.hpp"
#include "../BufferPool/Page.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

/**
 * Constructor - allocate buffer with configurable size
 */
MergeBuffer::MergeBuffer(size_t bufferSize)
    : capacity(bufferSize), currentPos(0), validPairs(0) {
    // Allocate interleaved array: each pair needs 2 int32_t elements
    buffer = new int32_t[capacity * 2];
    std::memset(buffer, 0, capacity * 2 * sizeof(int32_t));
}

/**
 * Destructor - deallocate buffer
 */
MergeBuffer::~MergeBuffer() {
    delete[] buffer;
}

// === Input Buffer Operations ===

/**
 * Refill buffer from SST file starting at given file offset
 * 
 * SST file structure:
 * - Page 0: Metadata
 * - Pages 1 to N: Internal nodes (if tree height > 1)
 * - Pages N+1 onwards: Leaf nodes with interleaved key-value pairs
 * 
 * The leaf section is a continuous stream of [k1,v1, k2,v2, ...] across pages
 */
bool MergeBuffer::refillFromSST(const std::string& sstFile, size_t& fileOffset) {
    // Open file for reading
    int fd = open(sstFile.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "Error: Cannot open SST file for reading: " << sstFile << std::endl;
        return false;
    }
    
    // Calculate how many bytes to read (capacity pairs * 2 elements * 4 bytes each)
    size_t bytesToRead = capacity * 2 * sizeof(int32_t);
    
    // Read data from current file offset
    ssize_t bytesRead = pread(fd, buffer, bytesToRead, fileOffset);
    close(fd);
    
    if (bytesRead < 0) {
        std::cerr << "Error: Failed to read from SST file: " << sstFile << std::endl;
        return false;
    }
    
    if (bytesRead == 0) {
        // End of file reached
        validPairs = 0;
        currentPos = 0;
        return false;
    }
    
    // Calculate how many complete pairs were read
    size_t elementsRead = bytesRead / sizeof(int32_t);
    validPairs = elementsRead / 2;  // Each pair is 2 elements
    currentPos = 0;
    
    // Update file offset for next read
    fileOffset += bytesRead;
    
    return validPairs > 0;
}

/**
 * Check if buffer contains unprocessed pairs
 */
bool MergeBuffer::hasData() const {
    return currentPos < validPairs;
}

/**
 * Peek at the minimum (next) key-value pair without consuming it
 */
std::pair<int32_t, int32_t> MergeBuffer::peekMin() const {
    if (!hasData()) {
        std::cerr << "Error: Attempting to peek from empty buffer" << std::endl;
        return {0, 0};
    }
    
    // Keys are at even indices, values at odd indices
    size_t keyIndex = currentPos * 2;
    size_t valueIndex = keyIndex + 1;
    
    return {buffer[keyIndex], buffer[valueIndex]};
}

/**
 * Consume the minimum pair by advancing the read position
 */
void MergeBuffer::consumeMin() {
    if (hasData()) {
        currentPos++;
    }
}

// === Output Buffer Operations ===

/**
 * Append a key-value pair to the output buffer
 */
bool MergeBuffer::append(int32_t key, int32_t value) {
    if (isFull()) {
        return false;
    }
    
    // Append at the end of valid pairs
    size_t keyIndex = validPairs * 2;
    size_t valueIndex = keyIndex + 1;
    
    buffer[keyIndex] = key;
    buffer[valueIndex] = value;
    validPairs++;
    
    return true;
}

/**
 * Check if buffer has reached capacity
 */
bool MergeBuffer::isFull() const {
    return validPairs >= capacity;
}

/**
 * Flush buffer contents to file descriptor
 */
bool MergeBuffer::flushToFile(int fd) {
    if (validPairs == 0) {
        return true;  // Nothing to flush
    }
    
    // Calculate bytes to write
    size_t bytesToWrite = validPairs * 2 * sizeof(int32_t);
    
    // Write buffer contents
    ssize_t bytesWritten = write(fd, buffer, bytesToWrite);
    
    if (bytesWritten != static_cast<ssize_t>(bytesToWrite)) {
        std::cerr << "Error: Failed to flush buffer (wrote " << bytesWritten 
                  << " bytes, expected " << bytesToWrite << ")" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * Clear buffer and reset for next batch
 */
void MergeBuffer::clear() {
    validPairs = 0;
    currentPos = 0;
    std::memset(buffer, 0, capacity * 2 * sizeof(int32_t));
}
