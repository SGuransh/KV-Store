#include "ClockEvictionPolicy.hpp"

ClockEvictionPolicy::ClockEvictionPolicy() : clockPointer(0) {
    // Initialize empty clock structure
}

PageID ClockEvictionPolicy::selectVictim() {
    if (clockList.empty()) {
        throw std::runtime_error("Cannot select victim from empty buffer");
    }

    // Handle single page case
    if (clockList.size() == 1) {
        return clockList[0].pageId;
    }

    // Clock algorithm: move hand until we find a page with reference bit = 0
    std::size_t startPosition = clockPointer;
    bool foundVictim = false;
    
    do {
        ClockEntry& currentEntry = clockList[clockPointer];
        
        if (!currentEntry.referenceBit) {
            // Found victim - page with reference bit = 0
            foundVictim = true;
            PageID victimId = currentEntry.pageId;
            
            // Move clock pointer to next position for next eviction
            clockPointer = (clockPointer + 1) % clockList.size();
            
            return victimId;
        } else {
            // Give second chance - clear reference bit and move to next
            currentEntry.referenceBit = false;
        }
        
        // Move clock hand to next position (circular)
        clockPointer = (clockPointer + 1) % clockList.size();
        
    } while (clockPointer != startPosition && !foundVictim);

    // If we've made a full circle without finding a victim,
    // all pages had reference bits set, but now they're all cleared.
    // Select the page at current position as victim.
    PageID victimId = clockList[clockPointer].pageId;
    clockPointer = (clockPointer + 1) % clockList.size();
    
    return victimId;
}

void ClockEvictionPolicy::recordAccess(const PageID& id) {
    std::string pageKey = id.toString();
    auto it = pageIndex.find(pageKey);
    
    if (it != pageIndex.end()) {
        // Page found - set reference bit
        clockList[it->second].referenceBit = true;
    }
    // If page not found, it's not in the buffer (ignore silently)
}

void ClockEvictionPolicy::recordInsertion(const PageID& id) {
    std::string pageKey = id.toString();
    
    // Check if page already exists (shouldn't happen, but be safe)
    if (pageIndex.find(pageKey) != pageIndex.end()) {
        return; // Page already tracked
    }
    
    // Add new page to clock structure
    std::size_t newIndex = clockList.size();
    clockList.emplace_back(id);
    pageIndex[pageKey] = newIndex;
    
    // If this is the first page, initialize clock pointer
    if (clockList.size() == 1) {
        clockPointer = 0;
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
    
    // Remove from clock list (swap with last element for O(1) removal)
    if (removeIndex < clockList.size() - 1) {
        // Update index for the page we're swapping
        std::string swappedPageKey = clockList.back().pageId.toString();
        pageIndex[swappedPageKey] = removeIndex;
        
        // Swap and pop
        clockList[removeIndex] = std::move(clockList.back());
    }
    clockList.pop_back();
    
    // Update clock pointer if necessary
    if (clockList.empty()) {
        clockPointer = 0;
    } else if (clockPointer >= clockList.size()) {
        clockPointer = 0; // Wrap around if pointer is beyond new size
    }
}

std::size_t ClockEvictionPolicy::getTrackedPageCount() const {
    return clockList.size();
}

bool ClockEvictionPolicy::isEmpty() const {
    return clockList.empty();
}

std::size_t ClockEvictionPolicy::findPageIndex(const PageID& id) const {
    std::string pageKey = id.toString();
    auto it = pageIndex.find(pageKey);
    return (it != pageIndex.end()) ? it->second : clockList.size();
}

void ClockEvictionPolicy::updateIndicesAfterRemoval(std::size_t removedIndex) {
    // Update all indices that are greater than the removed index
    for (auto& pair : pageIndex) {
        if (pair.second > removedIndex) {
            pair.second--;
        }
    }
}