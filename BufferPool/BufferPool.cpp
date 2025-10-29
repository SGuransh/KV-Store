#include "BufferPool.hpp"
#include <stdexcept>

BufferPool::BufferPool(std::size_t bufferSize, std::unique_ptr<EvictionPolicy> policy)
    : bufferSize(bufferSize), currentSize(0) {
    
    // Validate input parameters
    if (bufferSize == 0) {
        throw std::invalid_argument("Buffer size must be greater than 0");
    }
    
    if (!policy) {
        throw std::invalid_argument("Eviction policy cannot be null");
    }
    
    // Transfer ownership of the eviction policy
    evictionPolicy = std::move(policy);
    
    // Initialize the eviction policy with buffer capacity
    evictionPolicy->initialize(bufferSize);
    
    // Initialize hash table with appropriate number of buckets
    // Use buffer size * 1.3 to maintain good load factor
    std::size_t numBuckets = static_cast<std::size_t>(bufferSize * 1.3);
    table = std::make_unique<HashTable>(numBuckets);
}

std::size_t BufferPool::getCurrentSize() const {
    return currentSize;
}

std::size_t BufferPool::getBufferSize() const {
    return bufferSize;
}

bool BufferPool::isAtCapacity() const {
    return currentSize >= bufferSize;
}

bool BufferPool::isEmpty() const {
    return currentSize == 0;
}

double BufferPool::getLoadFactor() const {
    return table->getLoadFactor();
}

bool BufferPool::validateSizes() const {
    // Verify that our size tracking matches the hash table and eviction policy
    return (currentSize == table->getCurrentSize() && 
            currentSize == evictionPolicy->getTrackedPageCount());
}

Page* BufferPool::getPage(const PageID& id) {
    // Look up page in hash table
    Page* page = table->find(id);
    
    if (page != nullptr) {
        // Page found - record access in eviction policy
        evictionPolicy->recordAccess(id);
    }
    
    return page;
}

bool BufferPool::putPage(const PageID& id, const Page& pageData) {
    // Check if page already exists
    Page* existingPage = table->find(id);
    
    if (existingPage != nullptr) {
        // Page already exists - update it and record access
        *existingPage = pageData;
        evictionPolicy->recordAccess(id);
        return true;
    }
    
    // New page insertion - check if we need to evict
    if (isAtCapacity()) {
        // Buffer is full - need to evict a page first
        if (!evictPage()) {
            // Eviction failed - cannot insert new page
            return false;
        }
    }
    
    // Insert page into hash table
    bool inserted = table->insert(id, pageData);
    if (!inserted) {
        // Hash table insertion failed
        return false;
    }
    
    // Record insertion in eviction policy
    evictionPolicy->recordInsertion(id);
    
    // Update size
    ++currentSize;
    
    return true;
}

bool BufferPool::removePage(const PageID& id) {
    // Remove from hash table
    bool removed = table->remove(id);
    
    if (removed) {
        // Page was found and removed - update eviction policy and size
        evictionPolicy->recordRemoval(id);
        --currentSize;
    }
    
    return removed;
}

bool BufferPool::evictPage() {
    try {
        // Select victim page using eviction policy
        PageID victimId = evictionPolicy->selectVictim();
        
        // Remove from hash table
        bool removed = table->remove(victimId);
        if (!removed) {
            // This should not happen - eviction policy and hash table are out of sync
            return false;
        }
        
        // Remove from eviction policy tracking
        evictionPolicy->recordRemoval(victimId);
        
        // Update size
        --currentSize;
        
        return true;
    } catch (const std::exception&) {
        // Eviction failed (e.g., no pages to evict)
        return false;
    }
}