#include "Page.hpp"

Page::Page() {
    std::memset(data, 0, PAGE_SIZE);
}

Page::Page(const char* sourceData) {
    if (sourceData != nullptr) {
        std::memcpy(data, sourceData, PAGE_SIZE);
    } else {
        std::memset(data, 0, PAGE_SIZE);
    }
}

Page::Page(const Page& other) {
    std::memcpy(data, other.data, PAGE_SIZE);
}

Page& Page::operator=(const Page& other) {
    if (this != &other) {
        std::memcpy(data, other.data, PAGE_SIZE);
    }
    return *this;
}

void Page::clear() {
    std::memset(data, 0, PAGE_SIZE);
}

void Page::copyFrom(const char* sourceData) {
    if (sourceData != nullptr) {
        std::memcpy(data, sourceData, PAGE_SIZE);
    } else {
        std::memset(data, 0, PAGE_SIZE);
    }
}

char* Page::getData() {
    return data;
}

const char* Page::getData() const {
    return data;
}