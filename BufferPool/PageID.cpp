#include "PageID.hpp"

PageID::PageID(const std::string& file, std::size_t off) 
    : fileName(file), offset(off) {
}

PageID::PageID(const PageID& other) 
    : fileName(other.fileName), offset(other.offset) {
}

PageID& PageID::operator=(const PageID& other) {
    if (this != &other) {
        fileName = other.fileName;
        offset = other.offset;
    }
    return *this;
}

const std::string& PageID::getFileName() const {
    return fileName;
}

std::size_t PageID::getOffset() const {
    return offset;
}

std::string PageID::toString() const {
    return fileName + ":" + std::to_string(offset);
}

bool PageID::operator==(const PageID& other) const {
    return fileName == other.fileName && offset == other.offset;
}

bool PageID::operator!=(const PageID& other) const {
    return !(*this == other);
}