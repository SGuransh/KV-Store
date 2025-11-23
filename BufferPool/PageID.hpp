#pragma once

#ifndef PAGEID_HPP
#define PAGEID_HPP

#include <string>
#include <cstddef>

/**
 * PageID represents a unique identifier for a page in the buffer pool.
 * It combines a file name and an offset within that file to create
 * a unique identifier that can be used for hashing and comparison.
 */
class PageID {
private:
    std::string fileName;
    std::size_t offset;

public:
    /**
     * Constructor to create a PageID with file name and offset
     * @param file The name of the file containing the page
     * @param off The offset of the page within the file
     */
    PageID(const std::string& file, std::size_t off);

    /**
     * Copy constructor
     */
    PageID(const PageID& other);

    /**
     * Assignment operator
     */
    PageID& operator=(const PageID& other);

    /**
     * Get the file name
     * @return The file name as a string
     */
    const std::string& getFileName() const;

    /**
     * Get the offset
     * @return The offset within the file
     */
    std::size_t getOffset() const;

    /**
     * Convert PageID to string representation for hashing
     * Format: "fileName:offset"
     * @return String representation of the PageID
     */
    std::string toString() const;

    /**
     * Equality operator for comparing PageIDs
     * @param other The PageID to compare with
     * @return true if both file name and offset match
     */
    bool operator==(const PageID& other) const;

    /**
     * Inequality operator
     * @param other The PageID to compare with
     * @return true if file name or offset differ
     */
    bool operator!=(const PageID& other) const;
};

#endif // PAGEID_HPP