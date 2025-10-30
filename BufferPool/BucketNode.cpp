#include "BucketNode.hpp"

BucketNode::BucketNode(const PageID& pageId, const Page& pageData)
    : id(pageId), data(pageData), next(nullptr) {
}

BucketNode::BucketNode(const BucketNode& other)
    : id(other.id), data(other.data), next(nullptr) {
    // Note: We don't copy the next pointer to avoid deep copying chains
    // The caller is responsible for managing chain structure
}

BucketNode& BucketNode::operator=(const BucketNode& other) {
    if (this != &other) {
        id = other.id;
        data = other.data;
        // Note: We don't copy the next pointer to avoid deep copying chains
        // The caller is responsible for managing chain structure
        next = nullptr;
    }
    return *this;
}

const PageID& BucketNode::getPageID() const {
    return id;
}

const Page& BucketNode::getPageData() const {
    return data;
}

Page& BucketNode::getPageData() {
    return data;
}

BucketNode* BucketNode::getNext() const {
    return next;
}

void BucketNode::setNext(BucketNode* nextNode) {
    next = nextNode;
}