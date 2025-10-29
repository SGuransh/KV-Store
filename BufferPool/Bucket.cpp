#include "Bucket.hpp"

Bucket::Bucket() : head(nullptr) {
}

Bucket::~Bucket() {
    clear();
}

Bucket::Bucket(const Bucket& other) : head(nullptr) {
    copyChain(other);
}

Bucket& Bucket::operator=(const Bucket& other) {
    if (this != &other) {
        clear();
        copyChain(other);
    }
    return *this;
}

Page* Bucket::find(const PageID& pageId) {
    BucketNode* current = head;
    
    // Traverse the chain looking for matching PageID
    while (current != nullptr) {
        if (current->getPageID() == pageId) {
            return &(current->getPageData());
        }
        current = current->getNext();
    }
    
    return nullptr;  // Page not found
}

bool Bucket::insert(const PageID& pageId, const Page& pageData) {
    // First check if the page already exists
    BucketNode* current = head;
    while (current != nullptr) {
        if (current->getPageID() == pageId) {
            // Update existing page data
            current->getPageData() = pageData;
            return true;
        }
        current = current->getNext();
    }
    
    // Page doesn't exist, create new node and insert at head
    try {
        BucketNode* newNode = new BucketNode(pageId, pageData);
        newNode->setNext(head);
        head = newNode;
        return true;
    } catch (const std::bad_alloc&) {
        return false;  // Memory allocation failed
    }
}

bool Bucket::remove(const PageID& pageId) {
    if (head == nullptr) {
        return false;  // Empty bucket
    }
    
    // Special case: removing the head node
    if (head->getPageID() == pageId) {
        BucketNode* nodeToDelete = head;
        head = head->getNext();
        delete nodeToDelete;
        return true;
    }
    
    // Search for the node to remove
    BucketNode* current = head;
    while (current->getNext() != nullptr) {
        if (current->getNext()->getPageID() == pageId) {
            BucketNode* nodeToDelete = current->getNext();
            current->setNext(nodeToDelete->getNext());
            delete nodeToDelete;
            return true;
        }
        current = current->getNext();
    }
    
    return false;  // Page not found
}

BucketNode* Bucket::getHead() const {
    return head;
}

bool Bucket::isEmpty() const {
    return head == nullptr;
}

std::size_t Bucket::size() const {
    std::size_t count = 0;
    BucketNode* current = head;
    
    while (current != nullptr) {
        ++count;
        current = current->getNext();
    }
    
    return count;
}

void Bucket::clear() {
    while (head != nullptr) {
        BucketNode* nodeToDelete = head;
        head = head->getNext();
        delete nodeToDelete;
    }
}

void Bucket::copyChain(const Bucket& other) {
    if (other.head == nullptr) {
        return;  // Nothing to copy
    }
    
    // Copy nodes in reverse order to maintain original order
    BucketNode* otherCurrent = other.head;
    BucketNode* lastCopied = nullptr;
    
    try {
        // First, create the head node
        head = new BucketNode(otherCurrent->getPageID(), otherCurrent->getPageData());
        lastCopied = head;
        otherCurrent = otherCurrent->getNext();
        
        // Copy remaining nodes
        while (otherCurrent != nullptr) {
            BucketNode* newNode = new BucketNode(otherCurrent->getPageID(), otherCurrent->getPageData());
            lastCopied->setNext(newNode);
            lastCopied = newNode;
            otherCurrent = otherCurrent->getNext();
        }
    } catch (const std::bad_alloc&) {
        // Clean up partial copy on memory allocation failure
        clear();
        throw;
    }
}