#pragma once

#ifndef MURMURHASH_HPP
#define MURMURHASH_HPP

#include <string>
#include <cstdint>

/**
 * MurmurHash implements the MurmurHash3 32-bit hash function.
 * This is a non-cryptographic hash function known for good distribution
 * properties and performance, making it suitable for hash table implementations.
 * 
 * The implementation provides configurable seed values for hash randomization
 * and includes proper bit manipulation and finalization steps for optimal
 * distribution characteristics.
 */
class MurmurHash {
private:
    static constexpr std::uint32_t C1 = 0xcc9e2d51;
    static constexpr std::uint32_t C2 = 0x1b873593;
    static constexpr std::uint32_t R1 = 15;
    static constexpr std::uint32_t R2 = 13;
    static constexpr std::uint32_t M = 5;
    static constexpr std::uint32_t N = 0xe6546b64;

    /**
     * Rotate left operation for bit manipulation
     * @param value The value to rotate
     * @param shift Number of positions to rotate left
     * @return The rotated value
     */
    static std::uint32_t rotateLeft(std::uint32_t value, std::uint32_t shift);

    /**
     * Finalization mix function to ensure good distribution
     * @param hash The hash value to finalize
     * @return The finalized hash value
     */
    static std::uint32_t finalizationMix(std::uint32_t hash);

public:
    /**
     * Default constructor
     */
    MurmurHash() = default;

    /**
     * Compute 32-bit MurmurHash3 for string input
     * @param input The string to hash
     * @param seed The seed value for hash randomization (default: 0)
     * @return 32-bit hash value
     */
    std::uint32_t hash(const std::string& input, std::uint32_t seed = 0) const;

    /**
     * Compute hash for raw byte data
     * @param data Pointer to the data to hash
     * @param length Length of the data in bytes
     * @param seed The seed value for hash randomization (default: 0)
     * @return 32-bit hash value
     */
    std::uint32_t hash(const void* data, std::size_t length, std::uint32_t seed = 0) const;
};

#endif // MURMURHASH_HPP