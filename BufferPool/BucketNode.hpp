#pragma once

#ifndef BUCKETNODE_HPP
#define BUCKETNODE_HPP

#include "PageID.hpp"
#include "Page.hpp"

/**
 * BucketNode represents a single node in the chaining collision resolution
 * mechanism of the hash table. Each node contains a PageID, Page data,
 * and a pointer to the next node in the chain.
 */
class BucketNode {
public:
    PageID id;           // The page identifier
    Page data;           // The 4KB page data
    BucketNode* next;    // Pointer to next node in the chain

    /**
     * Constructor to create a BucketNode with PageID and Page data
     * @param pageId The PageID for this node
     * @param pageData The Page data for this node
     */
    BucketNode(const PageID& pageId, const Page& pageData);

    /**
     * Destructor
     */
    ~BucketNode() = default;

    /**
     * Copy constructor
     */
    BucketNode(const BucketNode& other);

    /**
     * Assignment operator
     */
    BucketNode& operator=(const BucketNode& other);

    /**
     * Get the PageID of this node
     * @return Reference to the PageID
     */
    const PageID& getPageID() const;

    /**
     * Get the Page data of this node
     * @return Reference to the Page data
     */
    const Page& getPageData() const;

    /**
     * Get mutable reference to Page data
     * @return Reference to the Page data
     */
    Page& getPageData();

    /**
     * Get the next node in the chain
     * @return Pointer to the next BucketNode, or nullptr if this is the last node
     */
    BucketNode* getNext() const;

    /**
     * Set the next node in the chain
     * @param nextNode Pointer to the next BucketNode
     */
    void setNext(BucketNode* nextNode);
};

#endif // BUCKETNODE_HPP