#include "MergeBuffer.hpp"
#include "../BTree/BTreeNode.hpp"
#include "../BufferPool/Page.hpp"
#include "../DBConfig.hpp"
#include "../BufferPool/BufferPool.hpp"
#include "../BufferPool/PageID.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

/**
 * Constructor - allocate buffer with configurable size
 */
MergeBuffer::MergeBuffer(size_t bufferSize, BufferPool* pool)
    : capacity(bufferSize), validPairs(0), currentPos(0), bufferPool(pool) {
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
 * Uses BufferPool if available, otherwise falls back to pread
 */
bool MergeBuffer::refillFromSST(const std::string& sstFile, size_t& fileOffset, size_t maxOffset) {
    // Check if we've reached the end of leaf data
    if (fileOffset >= maxOffset) {
        validPairs = 0;
        currentPos = 0;
        return false;
    }
    
    // Calculate how many bytes to read (capacity pairs * 2 elements * 4 bytes each)
    size_t bytesToRead = capacity * 2 * sizeof(int32_t);
    
    // Don't read past end of leaf data
    if (fileOffset + bytesToRead > maxOffset) {
        bytesToRead = maxOffset - fileOffset;
    }
    
    ssize_t bytesRead = 0;
    
    // Use BufferPool if available
    if (bufferPool != nullptr) {
        // Read using page-aligned BufferPool access
        size_t currentOffset = fileOffset;
        size_t totalBytesRead = 0;
        char* destPtr = reinterpret_cast<char*>(buffer);
        
        while (totalBytesRead < bytesToRead) {
            // Calculate which page we need
            size_t pageNumber = currentOffset / Page::PAGE_SIZE;
            size_t offsetInPage = currentOffset % Page::PAGE_SIZE;
            size_t bytesInThisPage = std::min(bytesToRead - totalBytesRead, Page::PAGE_SIZE - offsetInPage);
            
            // Create PageID for this page
            PageID pageId(sstFile, pageNumber * Page::PAGE_SIZE);
            
            // Try to get page from cache
            Page* cachedPage = bufferPool->getPage(pageId);
            
            if (cachedPage != nullptr) {
                // Cache hit - copy data from cached page
                std::memcpy(destPtr + totalBytesRead, cachedPage->getData() + offsetInPage, bytesInThisPage);
            } else {
                // Cache miss - load page from disk
                Page newPage;
                int fd = open(sstFile.c_str(), O_RDONLY | O_DIRECT);
                if (fd < 0) {
                    std::cerr << "Error: Cannot open SST file: " << sstFile << std::endl;
                    return false;
                }
                
                ssize_t pageBytes = pread(fd, newPage.getData(), Page::PAGE_SIZE, pageNumber * Page::PAGE_SIZE);
                close(fd);
                
                if (pageBytes <= 0) {
                    break;  // End of file or error
                }
                
                // Add page to buffer pool
                bufferPool->putPage(pageId, newPage);
                
                // Copy data from newly loaded page
                std::memcpy(destPtr + totalBytesRead, newPage.getData() + offsetInPage, bytesInThisPage);
            }
            
            totalBytesRead += bytesInThisPage;
            currentOffset += bytesInThisPage;
        }
        
        bytesRead = totalBytesRead;
    } else {
        // Fallback to direct pread if no BufferPool
        int fd = open(sstFile.c_str(), O_RDONLY | O_DIRECT);
        if (fd < 0) {
            std::cerr << "Error: Cannot open SST file for reading: " << sstFile << std::endl;
            return false;
        }
        
        bytesRead = pread(fd, buffer, bytesToRead, fileOffset);
        close(fd);
        
        if (bytesRead < 0) {
            std::cerr << "Error: Failed to read from SST file: " << sstFile << std::endl;
            return false;
        }
    }
    
    if (bytesRead == 0) {
        // End of leaf data reached
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
