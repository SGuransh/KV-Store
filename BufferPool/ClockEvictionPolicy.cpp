#include "ClockEvictionPolicy.hpp"

ClockEvictionPolicy::ClockEvictionPolicy() 
    : clockPointer(0), fillPointer(0), maxCapacity(0), currentCount(0) {
    // Constructor - actual initialization happens in initialize()
}

void ClockEvictionPolicy::initialize(std::size_t capacity) {
    maxCapacity = capacity;
    clockArray.resize(capacity);
    clockPointer = 0;
    fillPointer = 0;
    currentCount = 0;
    freeIndices.clear();
    pageIndex.clear();
}

PageID ClockEvictionPolicy::selectVictim() {
    if (currentCount == 0) {
        throw std::runtime_error("Cannot select victim from empty buffer");
    }

    // Ensure we start at a valid page
    if (!clockArray[clockPointer].isValid) {
        moveToNextValidPage();
    }
    std::size_t startPosition = clockPointer;
    
    // Clock algorithm: sweep until we find a victim
    do {
        ClockEntry& currentEntry = clockArray[clockPointer];
        
        if (!currentEntry.referenceBit) {
            // Found victim - return it and advance clock for next time
            PageID victimId = currentEntry.pageId;
            moveToNextValidPage();
            return victimId;
        }
        
        // Give second chance - clear reference bit and continue sweep
        currentEntry.referenceBit = false;
        moveToNextValidPage();
        
    } while (clockPointer != startPosition);

    // Full sweep completed - all pages got second chance, now all have referenceBit = false
    // The page we're currently at is our victim
    PageID victimId = clockArray[clockPointer].pageId;
    moveToNextValidPage();
    return victimId;
}

void ClockEvictionPolicy::recordAccess(const PageID& id) {
    std::string pageKey = id.toString();
    auto it = pageIndex.find(pageKey);
    
    if (it != pageIndex.end()) {
        // Page found - set reference bit (O(1) operation)
        std::size_t index = it->second;
        if (clockArray[index].isValid) {
            clockArray[index].referenceBit = true;
        }
    }
    // If page not found, it's not in the buffer (ignore silently)
}

void ClockEvictionPolicy::recordInsertion(const PageID& id) {
    std::string pageKey = id.toString();
    
    // Check if page already exists (shouldn't happen, but be safe)
    if (pageIndex.find(pageKey) != pageIndex.end()) {
        return; // Page already tracked
    }
    
    // Get next available index (O(1) operation)
    std::size_t newIndex = getNextAvailableIndex();
    
    // Add new page to clock array
    clockArray[newIndex] = ClockEntry(id);
    pageIndex[pageKey] = newIndex;
    ++currentCount;
    
    // If this is the first page, initialize clock pointer
    if (currentCount == 1) {
        clockPointer = newIndex;
    }
}

void ClockEvictionPolicy::recordRemoval(const PageID& id) {
    std::string pageKey = id.toString();
    auto it = pageIndex.find(pageKey);
    
    if (it == pageIndex.end()) {
        return; // Page not found (ignore silently)
    }
    
    std::size_t removeIndex = it->second;
    
    // Remove from index map
    pageIndex.erase(it);
    
    // Mark slot as invalid and add to free indices (O(1) operations)
    clockArray[removeIndex].isValid = false;
    freeIndices.push_back(removeIndex);
    --currentCount;
    
    // Update clock pointer if it's pointing to the removed page
    if (clockPointer == removeIndex && currentCount > 0) {
        // Find next valid page for clock pointer
        std::size_t startPos = clockPointer;
        do {
            clockPointer = (clockPointer + 1) % maxCapacity;
        } while (!clockArray[clockPointer].isValid && clockPointer != startPos);
    }
    
    // If buffer is now empty, reset clock pointer
    if (currentCount == 0) {
        clockPointer = 0;
    }
}

std::size_t ClockEvictionPolicy::getTrackedPageCount() const {
    return currentCount;
}

bool ClockEvictionPolicy::isEmpty() const {
    return currentCount == 0;
}

std::size_t ClockEvictionPolicy::getNextAvailableIndex() {
    // If we have free indices from previous removals, use them (O(1))
    if (!freeIndices.empty()) {
        std::size_t index = freeIndices.back();
        freeIndices.pop_back();
        return index;
    }
    
    // Otherwise, use the fill pointer (O(1))
    if (fillPointer < maxCapacity) {
        return fillPointer++;
    }
    
    // This should never happen if buffer management is correct
    throw std::runtime_error("No available indices - buffer overflow");
}

void ClockEvictionPolicy::moveToNextValidPage() {
    if (currentCount == 0) {
        return; // No valid pages to move to
    }
    
    std::size_t startPos = clockPointer;
    do {
        clockPointer = (clockPointer + 1) % maxCapacity;
    } while (!clockArray[clockPointer].isValid && clockPointer != startPos);
}