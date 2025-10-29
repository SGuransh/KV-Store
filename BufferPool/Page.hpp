#pragma once

#ifndef PAGE_HPP
#define PAGE_HPP

#include <cstring>
#include <cstddef>

/**
 * Page represents a 4KB block of data that is the fundamental unit
 * of storage and caching in the buffer pool system.
 */
struct Page {
    static constexpr std::size_t PAGE_SIZE = 4096;  // 4KB page size
    
    char data[PAGE_SIZE];  // 4KB data array

    /**
     * Default constructor - initializes page data to zero
     */
    Page();

    /**
     * Constructor that copies data from source array
     * @param sourceData Pointer to source data (must be at least PAGE_SIZE bytes)
     */
    explicit Page(const char* sourceData);

    /**
     * Copy constructor
     */
    Page(const Page& other);

    /**
     * Assignment operator
     */
    Page& operator=(const Page& other);

    /**
     * Clear the page data (set all bytes to zero)
     */
    void clear();

    /**
     * Copy data from source array into this page
     * @param sourceData Pointer to source data (must be at least PAGE_SIZE bytes)
     */
    void copyFrom(const char* sourceData);

    /**
     * Get pointer to the data array
     * @return Pointer to the internal data array
     */
    char* getData();

    /**
     * Get const pointer to the data array
     * @return Const pointer to the internal data array
     */
    const char* getData() const;
};

#endif // PAGE_HPP