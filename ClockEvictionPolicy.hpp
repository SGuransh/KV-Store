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
 * The Clock algorithm maintains a circular list of pages with reference bits.
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
        
        ClockEntry(const PageID& id) : pageId(id), referenceBit(true) {}
    };

    std::vector<ClockEntry> clockList;           // Circular list of pages
    std::size_t clockPointer;                    // Current position of clock hand
    std::unordered_map<std::string, std::size_t> pageIndex; // Maps PageID string to index in clockList

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
     * Helper method to find the index of a page in the clock list
     * 
     * @param id The PageID to find
     * @return Index in clockList, or clockList.size() if not found
     */
    std::size_t findPageIndex(const PageID& id) const;

    /**
     * Helper method to update page indices after removal
     * 
     * @param removedIndex The index that was removed
     */
    void updateIndicesAfterRemoval(std::size_t removedIndex);
};

#endif // CLOCKEVICTIONPOLICY_HPP