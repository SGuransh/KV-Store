#include "MurmurHash.hpp"
#include <cstring>

std::uint32_t MurmurHash::rotateLeft(std::uint32_t value, std::uint32_t shift) {
    return (value << shift) | (value >> (32 - shift));
}

std::uint32_t MurmurHash::finalizationMix(std::uint32_t hash) {
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}

std::uint32_t MurmurHash::hash(const std::string& input, std::uint32_t seed) const {
    return hash(input.c_str(), input.length(), seed);
}

std::uint32_t MurmurHash::hash(const void* data, std::size_t length, std::uint32_t seed) const {
    const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
    const std::size_t numBlocks = length / 4;
    
    std::uint32_t hash = seed;
    
    // Process 4-byte blocks
    for (std::size_t i = 0; i < numBlocks; ++i) {
        std::uint32_t block;
        std::memcpy(&block, bytes + i * 4, sizeof(block));
        
        block *= C1;
        block = rotateLeft(block, R1);
        block *= C2;
        
        hash ^= block;
        hash = rotateLeft(hash, R2);
        hash = hash * M + N;
    }
    
    // Process remaining bytes (less than 4)
    const std::uint8_t* tail = bytes + numBlocks * 4;
    std::uint32_t remainingBlock = 0;
    
    switch (length & 3) {
        case 3:
            remainingBlock ^= static_cast<std::uint32_t>(tail[2]) << 16;
            [[fallthrough]];
        case 2:
            remainingBlock ^= static_cast<std::uint32_t>(tail[1]) << 8;
            [[fallthrough]];
        case 1:
            remainingBlock ^= static_cast<std::uint32_t>(tail[0]);
            remainingBlock *= C1;
            remainingBlock = rotateLeft(remainingBlock, R1);
            remainingBlock *= C2;
            hash ^= remainingBlock;
            break;
        default:
            break;
    }
    
    // Finalization
    hash ^= static_cast<std::uint32_t>(length);
    hash = finalizationMix(hash);
    
    return hash;
}