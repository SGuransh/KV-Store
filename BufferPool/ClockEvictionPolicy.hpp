#pragma once

#ifndef CLOCKEVICTIONPOLICY_HPP
#define CLOCKEVICTIONPOLICY_HPP

#include "EvictionPolicy.hpp"
#include "PageID.hpp"
#include <vector>
#include <unordered_map>
#include <stdexcept>

/**
 * ClockEvictionPolicy implements the Clock page replacement algorithm.
 * This algorithm approximates LRU behavior using reference bits and a
 * circular pointer, providing efficient O(1) eviction decisions.
 * 
 * The Clock algorithm maintains a fixed-size circular array of pages with reference bits.
 * When a page needs to be evicted, the clock hand moves through the pages,
 * giving each page with a reference bit set a "second chance" by clearing
 * the bit. The first page found with a cleared reference bit is selected
 * for eviction.
 */
class ClockEvictionPolicy : public EvictionPolicy {
private:
    /**
     * ClockEntry represents a page in the clock structure
     */
    struct ClockEntry {
        PageID pageId;
        bool referenceBit;
        bool isValid;  // Whether this slot contains a valid page
        
        ClockEntry() : pageId("", 0), referenceBit(false), isValid(false) {}
        ClockEntry(const PageID& id) : pageId(id), referenceBit(true), isValid(true) {}
    };

    std::vector<ClockEntry> clockArray;          // Fixed-size circular array of pages
    std::vector<std::size_t> freeIndices;        // Stack of free indices for O(1) insertion after removal
    std::unordered_map<std::string, std::size_t> pageIndex; // Maps PageID string to index in clockArray
    
    std::size_t clockPointer;                    // Current position of clock hand
    std::size_t fillPointer;                     // Next position to fill when no free indices available
    std::size_t maxCapacity;                     // Maximum buffer capacity
    std::size_t currentCount;                    // Current number of valid pages

public:
    /**
     * Constructor initializes empty clock structure
     */
    ClockEvictionPolicy();

    /**
     * Destructor
     */
    virtual ~ClockEvictionPolicy() = default;

    /**
     * Initialize the eviction policy with the maximum buffer capacity.
     * Pre-allocates fixed-size data structures for O(1) operations.
     * 
     * @param maxCapacity Maximum number of pages the buffer can hold
     */
    virtual void initialize(std::size_t maxCapacity) override;

    /**
     * Select a victim page for eviction using the Clock algorithm.
     * Moves the clock hand through pages, giving second chances to
     * recently accessed pages by clearing their reference bits.
     * 
     * @return PageID of the page selected for eviction
     * @throws std::runtime_error if no pages are available for eviction
     */
    virtual PageID selectVictim() override;

    /**
     * Record that a page has been accessed by setting its reference bit.
     * 
     * @param id The PageID of the accessed page
     */
    virtual void recordAccess(const PageID& id) override;

    /**
     * Record that a new page has been inserted into the buffer pool.
     * Adds the page to the clock structure with reference bit set.
     * 
     * @param id The PageID of the newly inserted page
     */
    virtual void recordInsertion(const PageID& id) override;

    /**
     * Record that a page has been removed from the buffer pool.
     * Removes the page from the clock structure and updates indices.
     * 
     * @param id The PageID of the removed page
     */
    virtual void recordRemoval(const PageID& id) override;

    /**
     * Get the current number of pages being tracked by the clock.
     * 
     * @return Number of pages currently in the clock structure
     */
    virtual std::size_t getTrackedPageCount() const override;

    /**
     * Check if the clock is currently empty.
     * 
     * @return true if no pages are being tracked, false otherwise
     */
    virtual bool isEmpty() const override;

private:
    /**
     * Helper method to get the next available index for insertion
     * 
     * @return Index where the next page should be inserted
     */
    std::size_t getNextAvailableIndex();

    /**
     * Helper method to move clock pointer to the next valid page
     * Skips invalid entries in the circular array
     */
    void moveToNextValidPage();
};

#endif // CLOCKEVICTIONPOLICY_HPP