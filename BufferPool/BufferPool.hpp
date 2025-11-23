#pragma once

#ifndef BUFFERPOOL_HPP
#define BUFFERPOOL_HPP

#include "HashTable.hpp"
#include "EvictionPolicy.hpp"
#include "PageID.hpp"
#include "Page.hpp"
#include <memory>
#include <cstddef>

/**
 * BufferPool implements a memory cache system that stores frequently accessed
 * 4KB pages to reduce disk I/O operations. It uses a hash table for O(1) expected
 * access time and integrates with configurable eviction policies for memory management.
 */
class BufferPool {
private:
    std::unique_ptr<HashTable> table;                    // Hash table for page storage
    std::unique_ptr<EvictionPolicy> evictionPolicy;     // Eviction policy for memory management
    std::size_t bufferSize;                              // Maximum number of pages that can be cached
    std::size_t currentSize;                             // Current number of pages in the buffer

public:
    /**
     * Constructor to create a BufferPool with specified buffer size and eviction policy
     * @param bufferSize Maximum number of 4KB pages that can be cached
     * @param policy Unique pointer to the eviction policy to use (ownership transferred)
     */
    BufferPool(std::size_t bufferSize, std::unique_ptr<EvictionPolicy> policy);

    /**
     * Destructor
     */
    ~BufferPool() = default;

    /**
     * Disable copy constructor and assignment operator to prevent accidental copying
     * of the buffer pool (which contains unique_ptr members)
     */
    BufferPool(const BufferPool&) = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    /**
     * Retrieve a page from the buffer pool by PageID
     * Updates access tracking in the eviction policy
     * @param id The PageID of the page to retrieve
     * @return Pointer to the Page if found, nullptr otherwise
     */
    Page* getPage(const PageID& id);

    /**
     * Insert a page into the buffer pool
     * Handles eviction if the buffer is at capacity
     * @param id The PageID for the page
     * @param pageData The Page data to insert
     * @return true if insertion was successful, false otherwise
     */
    bool putPage(const PageID& id, const Page& pageData);

    /**
     * Remove a page from the buffer pool by PageID
     * Updates both hash table and eviction policy
     * @param id The PageID of the page to remove
     * @return true if the page was found and removed, false otherwise
     */
    bool removePage(const PageID& id);

    /**
     * Get the current number of pages in the buffer pool
     * @return The current size
     */
    std::size_t getCurrentSize() const;

    /**
     * Get the maximum buffer size (capacity)
     * @return The buffer size limit
     */
    std::size_t getBufferSize() const;

    /**
     * Check if the buffer pool is at capacity
     * @return true if currentSize equals bufferSize, false otherwise
     */
    bool isAtCapacity() const;

    /**
     * Check if the buffer pool is empty
     * @return true if no pages are cached, false otherwise
     */
    bool isEmpty() const;

    /**
     * Get the current load factor of the underlying hash table
     * @return Load factor (pages / buckets)
     */
    double getLoadFactor() const;

private:
    /**
     * Helper method to handle eviction when buffer is at capacity
     * @return true if eviction was successful, false otherwise
     */
    bool evictPage();

    /**
     * Helper method to validate buffer size consistency
     * @return true if sizes are consistent, false otherwise
     */
    bool validateSizes() const;
};

#endif // BUFFERPOOL_HPP