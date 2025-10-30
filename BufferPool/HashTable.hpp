#pragma once

#ifndef HASHTABLE_HPP
#define HASHTABLE_HPP

#include "Bucket.hpp"
#include "MurmurHash.hpp"
#include "PageID.hpp"
#include "Page.hpp"
#include <cstddef>
#include <memory>

/**
 * HashTable implements a hash table with chaining collision resolution
 * for the buffer pool system. It uses MurmurHash for distributing PageIDs
 * across buckets and provides O(1) expected performance for find, insert,
 * and remove operations.
 */
class HashTable {
private:
    std::unique_ptr<Bucket[]> buckets;  // Array of buckets
    std::size_t numBuckets;             // Number of buckets in the table
    MurmurHash hasher;                  // Hash function instance
    std::size_t currentSize;            // Current number of pages stored

    /**
     * Helper method to find the next power of two greater than or equal to n
     * @param n The starting number
     * @return The next power of two >= n
     */
    static std::size_t nextPowerOfTwo(std::size_t n);

public:
    /**
     * Constructor to create a HashTable with specified number of buckets
     * The actual number of buckets will be the next prime >= numBuckets
     * @param numBuckets The desired number of buckets (will be adjusted to next prime)
     */
    explicit HashTable(std::size_t numBuckets = 101);

    /**
     * Destructor
     */
    ~HashTable() = default;

    /**
     * Copy constructor
     */
    HashTable(const HashTable& other);

    /**
     * Assignment operator
     */
    HashTable& operator=(const HashTable& other);

    /**
     * Find a page in the hash table by PageID
     * @param pageId The PageID to search for
     * @return Pointer to the Page if found, nullptr otherwise
     */
    Page* find(const PageID& pageId);

    /**
     * Insert a page into the hash table
     * If a page with the same PageID already exists, it will be updated
     * @param pageId The PageID for the page
     * @param pageData The Page data to insert
     * @return true if insertion was successful, false otherwise
     */
    bool insert(const PageID& pageId, const Page& pageData);

    /**
     * Remove a page from the hash table by PageID
     * @param pageId The PageID of the page to remove
     * @return true if the page was found and removed, false otherwise
     */
    bool remove(const PageID& pageId);

    /**
     * Compute hash value for a PageID and map to bucket index
     * @param pageId The PageID to hash
     * @return The bucket index (0 to numBuckets-1)
     */
    std::size_t hash(const PageID& pageId) const;

    /**
     * Get the number of buckets in the hash table
     * @return The number of buckets
     */
    std::size_t getNumBuckets() const;

    /**
     * Get the current number of pages stored in the hash table
     * @return The current size
     */
    std::size_t getCurrentSize() const;

    /**
     * Get the load factor of the hash table
     * @return The load factor (currentSize / numBuckets)
     */
    double getLoadFactor() const;

    /**
     * Check if the hash table is empty
     * @return true if no pages are stored, false otherwise
     */
    bool isEmpty() const;

    /**
     * Get statistics about the hash table for debugging/monitoring
     * @param maxChainLength Reference to store the maximum chain length
     * @param avgChainLength Reference to store the average chain length
     * @param emptyBuckets Reference to store the number of empty buckets
     */
    void getStatistics(std::size_t& maxChainLength, double& avgChainLength, std::size_t& emptyBuckets) const;
};

#endif // HASHTABLE_HPP