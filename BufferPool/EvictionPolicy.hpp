#pragma once

#ifndef EVICTIONPOLICY_HPP
#define EVICTIONPOLICY_HPP

#include "PageID.hpp"

/**
 * EvictionPolicy defines the interface for page replacement algorithms
 * in the buffer pool system. This interface enables clean separation
 * between the core buffer pool logic and the eviction strategy,
 * allowing different eviction policies to be implemented and swapped
 * without affecting the core functionality.
 */
class EvictionPolicy {
public:
    /**
     * Virtual destructor to ensure proper cleanup of derived classes
     */
    virtual ~EvictionPolicy() = default;

    /**
     * Initialize the eviction policy with the maximum buffer capacity.
     * This allows the policy to pre-allocate fixed-size data structures.
     * 
     * @param maxCapacity Maximum number of pages the buffer can hold
     */
    virtual void initialize(std::size_t maxCapacity) = 0;

    /**
     * Select a victim page for eviction when the buffer pool reaches capacity.
     * This method should implement the specific eviction algorithm logic.
     * 
     * @return PageID of the page selected for eviction
     * @throws std::runtime_error if no page can be evicted (e.g., empty buffer)
     */
    virtual PageID selectVictim() = 0;

    /**
     * Record that a page has been accessed, updating any internal tracking
     * structures used by the eviction algorithm (e.g., reference bits, timestamps).
     * 
     * @param id The PageID of the accessed page
     */
    virtual void recordAccess(const PageID& id) = 0;

    /**
     * Record that a new page has been inserted into the buffer pool.
     * This allows the eviction policy to add the page to its tracking structures.
     * 
     * @param id The PageID of the newly inserted page
     */
    virtual void recordInsertion(const PageID& id) = 0;

    /**
     * Record that a page has been removed from the buffer pool.
     * This allows the eviction policy to clean up any tracking structures
     * associated with the removed page.
     * 
     * @param id The PageID of the removed page
     */
    virtual void recordRemoval(const PageID& id) = 0;

    /**
     * Get the current number of pages being tracked by the eviction policy.
     * This should match the number of pages currently in the buffer pool.
     * 
     * @return Number of pages currently tracked
     */
    virtual std::size_t getTrackedPageCount() const = 0;

    /**
     * Check if the eviction policy is currently tracking any pages.
     * 
     * @return true if no pages are being tracked, false otherwise
     */
    virtual bool isEmpty() const = 0;
};

#endif // EVICTIONPOLICY_HPP