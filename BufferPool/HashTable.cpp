#include "HashTable.hpp"
#include <algorithm>
#include <cmath>

HashTable::HashTable(std::size_t numBuckets) 
    : numBuckets(nextPowerOfTwo(numBuckets)), currentSize(0) {
    buckets = std::make_unique<Bucket[]>(this->numBuckets);
}

HashTable::HashTable(const HashTable& other) 
    : numBuckets(other.numBuckets), currentSize(other.currentSize) {
    buckets = std::make_unique<Bucket[]>(numBuckets);
    
    // Copy all buckets
    for (std::size_t i = 0; i < numBuckets; ++i) {
        buckets[i] = other.buckets[i];
    }
}

HashTable& HashTable::operator=(const HashTable& other) {
    if (this != &other) {
        numBuckets = other.numBuckets;
        currentSize = other.currentSize;
        buckets = std::make_unique<Bucket[]>(numBuckets);
        
        // Copy all buckets
        for (std::size_t i = 0; i < numBuckets; ++i) {
            buckets[i] = other.buckets[i];
        }
    }
    return *this;
}

Page* HashTable::find(const PageID& pageId) {
    std::size_t bucketIndex = hash(pageId);
    return buckets[bucketIndex].find(pageId);
}

bool HashTable::insert(const PageID& pageId, const Page& pageData) {
    std::size_t bucketIndex = hash(pageId);
    
    // Check if page already exists
    bool pageExists = (buckets[bucketIndex].find(pageId) != nullptr);
    
    bool success = buckets[bucketIndex].insert(pageId, pageData);
    
    // If insertion was successful and it's a new page, increment size
    if (success && !pageExists) {
        ++currentSize;
    }
    
    return success;
}

bool HashTable::remove(const PageID& pageId) {
    std::size_t bucketIndex = hash(pageId);
    bool success = buckets[bucketIndex].remove(pageId);
    
    // If removal was successful, decrement size
    if (success) {
        --currentSize;
    }
    
    return success;
}

std::size_t HashTable::hash(const PageID& pageId) const {
    std::string pageIdStr = pageId.toString();
    std::uint32_t hashValue = hasher.hash(pageIdStr);
    return hashValue % numBuckets;
}

std::size_t HashTable::getNumBuckets() const {
    return numBuckets;
}

std::size_t HashTable::getCurrentSize() const {
    return currentSize;
}

double HashTable::getLoadFactor() const {
    return static_cast<double>(currentSize) / static_cast<double>(numBuckets);
}

bool HashTable::isEmpty() const {
    return currentSize == 0;
}

void HashTable::getStatistics(std::size_t& maxChainLength, double& avgChainLength, std::size_t& emptyBuckets) const {
    maxChainLength = 0;
    std::size_t totalChainLength = 0;
    emptyBuckets = 0;
    
    for (std::size_t i = 0; i < numBuckets; ++i) {
        std::size_t chainLength = buckets[i].size();
        
        if (chainLength == 0) {
            ++emptyBuckets;
        } else {
            maxChainLength = std::max(maxChainLength, chainLength);
            totalChainLength += chainLength;
        }
    }
    
    std::size_t nonEmptyBuckets = numBuckets - emptyBuckets;
    avgChainLength = (nonEmptyBuckets > 0) ? 
        static_cast<double>(totalChainLength) / static_cast<double>(nonEmptyBuckets) : 0.0;
}

std::size_t HashTable::nextPowerOfTwo(std::size_t n) {
    // If n is 0 or 1, return 1
    if (n <= 1) return 1;
    
    // If n is already a power of 2, return n
    if ((n & (n - 1)) == 0) return n;
    
    // Round up to next power of 2 using bit manipulation
    --n;
    n |= n >> 1;   // Fill 2 bits
    n |= n >> 2;   // Fill 4 bits
    n |= n >> 4;   // Fill 8 bits
    n |= n >> 8;   // Fill 16 bits
    n |= n >> 16;  // Fill 32 bits
    n |= n >> 32;  // Fill 64 bits (for 64-bit size_t)
    ++n;
    
    return n;
}