#pragma once

#ifndef BUCKET_HPP
#define BUCKET_HPP

#include "BucketNode.hpp"
#include "PageID.hpp"
#include "Page.hpp"

/**
 * Bucket manages a chain of BucketNodes for collision resolution in the hash table.
 * Each bucket maintains a linked list of nodes that hash to the same bucket index.
 * The class provides methods for finding, inserting, and removing pages while
 * maintaining chain integrity.
 */
class Bucket {
private:
    BucketNode* head;  // Pointer to the first node in the chain

public:
    /**
     * Default constructor - creates an empty bucket
     */
    Bucket();

    /**
     * Destructor - cleans up all nodes in the chain
     */
    ~Bucket();

    /**
     * Copy constructor
     */
    Bucket(const Bucket& other);

    /**
     * Assignment operator
     */
    Bucket& operator=(const Bucket& other);

    /**
     * Find a page in the bucket chain by PageID
     * @param pageId The PageID to search for
     * @return Pointer to the Page if found, nullptr otherwise
     */
    Page* find(const PageID& pageId);

    /**
     * Insert a new page into the bucket chain
     * If a page with the same PageID already exists, it will be updated
     * @param pageId The PageID for the new page
     * @param pageData The Page data to insert
     * @return true if insertion was successful, false otherwise
     */
    bool insert(const PageID& pageId, const Page& pageData);

    /**
     * Remove a page from the bucket chain by PageID
     * @param pageId The PageID of the page to remove
     * @return true if the page was found and removed, false otherwise
     */
    bool remove(const PageID& pageId);

    /**
     * Get the head node of the chain (for iteration purposes)
     * @return Pointer to the first BucketNode, or nullptr if bucket is empty
     */
    BucketNode* getHead() const;

    /**
     * Check if the bucket is empty
     * @return true if the bucket contains no nodes, false otherwise
     */
    bool isEmpty() const;

    /**
     * Get the number of nodes in the chain
     * @return The count of nodes in this bucket
     */
    std::size_t size() const;

private:
    /**
     * Helper method to clear all nodes in the chain
     */
    void clear();

    /**
     * Helper method to deep copy a chain from another bucket
     * @param other The bucket to copy from
     */
    void copyChain(const Bucket& other);
};

#endif // BUCKET_HPP