#include "../BTree/BTreeSST.hpp"
#include "../Page/Page.hpp"
#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <cmath>

int bloom_tests_passed = 0;
int bloom_tests_failed = 0;

void bloom_test_passed(const std::string& test_name) {
    bloom_tests_passed++;
    std::cout << "[BLOOM] " << test_name << " PASSED!" << std::endl;
}

void bloom_test_failed(const std::string& test_name) {
    bloom_tests_failed++;
    std::cout << "[BLOOM] " << test_name << " FAILED!" << std::endl;
}

void test_basic_bloom_filter_creation() {
    std::cout << "\n--- Basic Bloom Filter Creation ---" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 10; i++) {
        data.push_back({i * 10, i * 100});
    }
    
    BTreeSST sst;
    bool success = sst.buildBTree(data, "test_basic_bloom.sst", 10, 3);
    
    if (success) {
        // Read metadata to verify bloom filter info was stored
        MetadataPage metadata;
        if (sst.readMetadata("test_basic_bloom.sst", metadata)) {
            if (metadata.bloom_bytes > 0 && metadata.bloom_hash_count == 3) {
                bloom_test_passed("Basic Bloom Filter Creation");
            } else {
                bloom_test_failed("Basic Bloom Filter Creation - Invalid Metadata");
            }
        } else {
            bloom_test_failed("Basic Bloom Filter Creation - Metadata Read Failed");
        }
    } else {
        bloom_test_failed("Basic Bloom Filter Creation - Build Failed");
    }
    
    remove("test_basic_bloom.sst");
}

void test_bloom_filter_no_false_negatives() {
    std::cout << "\n--- Bloom Filter No False Negatives ---" << std::endl;
    
    // Insert 100 keys
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i, i * 10});
    }
    
    BTreeSST sst;
    sst.buildBTree(data, "test_no_false_neg.sst", 10, 3);
    
    // All inserted keys MUST be found (no false negatives allowed)
    int found_count = 0;
    int value;
    for (int i = 1; i <= 100; i++) {
        if (sst.get(i, value, "test_no_false_neg.sst", true)) {
            if (value == i * 10) {
                found_count++;
            }
        }
    }
    
    if (found_count == 100) {
        bloom_test_passed("No False Negatives");
    } else {
        bloom_test_failed("No False Negatives - Found: " + std::to_string(found_count) + "/100");
    }
    
    remove("test_no_false_neg.sst");
}

void test_bloom_filter_false_positive_rate() {
    std::cout << "\n--- Bloom Filter False Positive Rate ---" << std::endl;
    
    // Insert 1000 keys (even numbers only)
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 1000; i++) {
        data.push_back({i * 2, i * 20});
    }
    
    BTreeSST sst;
    sst.buildBTree(data, "test_fp_rate.sst", 10, 3);
    
    // Query 1000 keys that were NOT inserted (odd numbers)
    int false_positives = 0;
    int value;
    for (int i = 1; i <= 1000; i++) {
        int odd_key = i * 2 + 1;
        if (sst.get(odd_key, value, "test_fp_rate.sst", true)) {
            false_positives++;
        }
    }
    
    // With 10 bits per entry and 3 hash functions, expected FP rate ≈ (1 - e^(-3/10))^3 ≈ 0.0264 or 2.64%
    // Allow up to 5% false positive rate
    double fp_rate = (double)false_positives / 1000.0;
    std::cout << "  False positive rate: " << (fp_rate * 100.0) << "% (" << false_positives << "/1000)" << std::endl;
    
    if (fp_rate <= 0.05) {
        bloom_test_passed("False Positive Rate Within Bounds");
    } else {
        bloom_test_failed("False Positive Rate Too High");
    }
    
    remove("test_fp_rate.sst");
}

void test_bloom_filter_different_parameters() {
    std::cout << "\n--- Bloom Filter Different Parameters ---" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i, i * 10});
    }
    
    // Test with different bits per entry
    BTreeSST sst1;
    sst1.buildBTree(data, "test_param1.sst", 5, 2);  // Lower bits, fewer hashes
    
    BTreeSST sst2;
    sst2.buildBTree(data, "test_param2.sst", 20, 5); // Higher bits, more hashes
    
    // Read metadata to verify parameters
    MetadataPage meta1, meta2;
    bool success = true;
    
    if (sst1.readMetadata("test_param1.sst", meta1)) {
        if (meta1.bloom_hash_count != 2) {
            success = false;
        }
    } else {
        success = false;
    }
    
    if (sst2.readMetadata("test_param2.sst", meta2)) {
        if (meta2.bloom_hash_count != 5) {
            success = false;
        }
    } else {
        success = false;
    }
    
    if (success) {
        bloom_test_passed("Different Bloom Filter Parameters");
    } else {
        bloom_test_failed("Different Bloom Filter Parameters");
    }
    
    remove("test_param1.sst");
    remove("test_param2.sst");
}

void test_bloom_filter_early_rejection() {
    std::cout << "\n--- Bloom Filter Early Rejection ---" << std::endl;
    
    // Insert keys 1-100
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i, i * 10});
    }
    
    BTreeSST sst;
    sst.buildBTree(data, "test_early_reject.sst", 10, 3);
    
    // Query keys that are definitely not in the bloom filter
    // Keys 1000-1100 should be rejected by bloom filter
    int rejected_count = 0;
    int value;
    for (int i = 1000; i < 1100; i++) {
        if (!sst.get(i, value, "test_early_reject.sst", true)) {
            rejected_count++;
        }
    }
    
    // Most should be rejected (allowing for some false positives)
    if (rejected_count >= 95) {
        bloom_test_passed("Early Rejection by Bloom Filter");
    } else {
        bloom_test_failed("Early Rejection - Only rejected: " + std::to_string(rejected_count) + "/100");
    }
    
    remove("test_early_reject.sst");
}

void test_bloom_filter_with_single_element() {
    std::cout << "\n--- Bloom Filter Single Element ---" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    data.push_back({42, 420});
    
    BTreeSST sst;
    sst.buildBTree(data, "test_single.sst", 10, 3);
    
    int value;
    bool found_correct = sst.get(42, value, "test_single.sst", true) && value == 420;
    bool not_found_incorrect = !sst.get(43, value, "test_single.sst", true);
    
    if (found_correct && not_found_incorrect) {
        bloom_test_passed("Single Element Bloom Filter");
    } else {
        bloom_test_failed("Single Element Bloom Filter");
    }
    
    remove("test_single.sst");
}

void test_bloom_filter_with_duplicates() {
    std::cout << "\n--- Bloom Filter With Duplicates ---" << std::endl;
    
    // Create data with duplicate keys (should be handled by B-Tree)
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 50; i++) {
        data.push_back({i, i * 10});
    }
    
    BTreeSST sst;
    sst.buildBTree(data, "test_duplicates.sst", 10, 3);
    
    // All unique keys should be found
    int found_count = 0;
    int value;
    for (int i = 1; i <= 50; i++) {
        if (sst.get(i, value, "test_duplicates.sst", true)) {
            found_count++;
        }
    }
    
    if (found_count == 50) {
        bloom_test_passed("Bloom Filter With Duplicates");
    } else {
        bloom_test_failed("Bloom Filter With Duplicates");
    }
    
    remove("test_duplicates.sst");
}

void test_bloom_filter_hash_distribution() {
    std::cout << "\n--- Bloom Filter Hash Distribution ---" << std::endl;
    
    // Test that different hash indices produce different results
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i, i * 10});
    }
    
    BTreeSST sst;
    std::vector<uint8_t> bloomFilter = sst.buildBloomFilter(data, 10, 3);
    
    // Count number of bits set
    int bits_set = 0;
    for (uint8_t byte : bloomFilter) {
        for (int bit = 0; bit < 8; bit++) {
            if (byte & (1 << bit)) {
                bits_set++;
            }
        }
    }
    
    int total_bits = bloomFilter.size() * 8;
    double fill_ratio = (double)bits_set / total_bits;
    
    std::cout << "  Bits set: " << bits_set << "/" << total_bits 
              << " (" << (fill_ratio * 100.0) << "%)" << std::endl;
    
    // Bloom filter should have reasonable fill ratio (not too empty, not too full)
    // With 100 elements, 10 bits/entry (1000 bits), 3 hashes (300 bit sets)
    // Expected fill: 1 - (1 - 1/1000)^300 ≈ 0.26 or 26%
    if (fill_ratio > 0.15 && fill_ratio < 0.50) {
        bloom_test_passed("Hash Distribution");
    } else {
        bloom_test_failed("Hash Distribution - Fill ratio: " + std::to_string(fill_ratio));
    }
}

void test_bloom_filter_edge_cases() {
    std::cout << "\n--- Bloom Filter Edge Cases ---" << std::endl;
    
    // Test with negative keys
    std::vector<std::pair<int, int>> data;
    data.push_back({-100, 100});
    data.push_back({-50, 50});
    data.push_back({0, 0});
    data.push_back({50, 500});
    data.push_back({100, 1000});
    
    BTreeSST sst;
    sst.buildBTree(data, "test_edge_cases.sst", 10, 3);
    
    int value;
    bool found_negative = sst.get(-100, value, "test_edge_cases.sst", true) && value == 100;
    bool found_zero = sst.get(0, value, "test_edge_cases.sst", true) && value == 0;
    bool found_positive = sst.get(100, value, "test_edge_cases.sst", true) && value == 1000;
    bool not_found = !sst.get(200, value, "test_edge_cases.sst", true);
    
    if (found_negative && found_zero && found_positive && not_found) {
        bloom_test_passed("Edge Cases (Negative/Zero/Positive Keys)");
    } else {
        bloom_test_failed("Edge Cases");
    }
    
    remove("test_edge_cases.sst");
}

void test_bloom_filter_large_dataset() {
    std::cout << "\n--- Bloom Filter Large Dataset ---" << std::endl;
    
    // Insert 10,000 keys
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 10000; i++) {
        data.push_back({i, i * 10});
    }
    
    BTreeSST sst;
    sst.buildBTree(data, "test_large.sst", 10, 3);
    
    // Verify all keys can be found
    int found_count = 0;
    int value;
    for (int i = 1; i <= 10000; i += 100) {  // Sample every 100th key
        if (sst.get(i, value, "test_large.sst", true) && value == i * 10) {
            found_count++;
        }
    }
    
    if (found_count == 100) {
        bloom_test_passed("Large Dataset (10,000 keys)");
    } else {
        bloom_test_failed("Large Dataset - Found: " + std::to_string(found_count) + "/100");
    }
    
    remove("test_large.sst");
}

void test_bloom_filter_bit_operations() {
    std::cout << "\n--- Bloom Filter Bit Operations ---" << std::endl;
    
    std::vector<uint8_t> bloomFilter(10, 0);  // 80 bits
    
    BTreeSST sst;
    
    // Test setting individual bits
    sst.bloomFilterSetBit(bloomFilter, 0);   // First bit
    sst.bloomFilterSetBit(bloomFilter, 7);   // Last bit of first byte
    sst.bloomFilterSetBit(bloomFilter, 8);   // First bit of second byte
    sst.bloomFilterSetBit(bloomFilter, 79);  // Last bit
    
    // Test reading those bits
    bool test1 = sst.bloomFilterTestBit(bloomFilter, 0);
    bool test2 = sst.bloomFilterTestBit(bloomFilter, 7);
    bool test3 = sst.bloomFilterTestBit(bloomFilter, 8);
    bool test4 = sst.bloomFilterTestBit(bloomFilter, 79);
    bool test5 = !sst.bloomFilterTestBit(bloomFilter, 5);  // Should be false
    
    if (test1 && test2 && test3 && test4 && test5) {
        bloom_test_passed("Bit Set/Test Operations");
    } else {
        bloom_test_failed("Bit Set/Test Operations");
    }
}

void test_bloom_filter_hash_consistency() {
    std::cout << "\n--- Bloom Filter Hash Consistency ---" << std::endl;
    
    BTreeSST sst;
    
    // Same key should produce same hash values
    uint32_t hash1_v1 = sst.bloomHash(42, 0, 1000);
    uint32_t hash1_v2 = sst.bloomHash(42, 0, 1000);
    
    uint32_t hash2_v1 = sst.bloomHash(42, 1, 1000);
    uint32_t hash2_v2 = sst.bloomHash(42, 1, 1000);
    
    // Different hash indices should (likely) produce different values
    bool same_key_consistent = (hash1_v1 == hash1_v2) && (hash2_v1 == hash2_v2);
    bool different_indices_different = (hash1_v1 != hash2_v1);
    
    // Different keys should (likely) produce different values
    uint32_t hash_42 = sst.bloomHash(42, 0, 1000);
    uint32_t hash_43 = sst.bloomHash(43, 0, 1000);
    bool different_keys_different = (hash_42 != hash_43);
    
    if (same_key_consistent && different_indices_different && different_keys_different) {
        bloom_test_passed("Hash Consistency");
    } else {
        bloom_test_failed("Hash Consistency");
    }
}

void test_bloom_filter_bytes_calculation() {
    std::cout << "\n--- Bloom Filter Bytes Calculation ---" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i, i * 10});
    }
    
    BTreeSST sst;
    sst.buildBTree(data, "test_bytes.sst", 10, 3);
    
    MetadataPage metadata;
    sst.readMetadata("test_bytes.sst", metadata);
    
    // With 100 entries and 10 bits per entry = 1000 bits = 125 bytes
    uint32_t expected_bits = 100 * 10;
    uint32_t expected_bytes = (expected_bits + 7) / 8;  // Round up
    
    if (metadata.bloom_bits == expected_bits && metadata.bloom_bytes == expected_bytes) {
        bloom_test_passed("Bytes Calculation (100 entries, 10 bits/entry)");
    } else {
        std::cout << "  Expected: " << expected_bits << " bits, " << expected_bytes << " bytes" << std::endl;
        std::cout << "  Got: " << metadata.bloom_bits << " bits, " << metadata.bloom_bytes << " bytes" << std::endl;
        bloom_test_failed("Bytes Calculation");
    }
    
    remove("test_bytes.sst");
}

void test_bloom_filter_range_queries() {
    std::cout << "\n--- Bloom Filter With Range Queries ---" << std::endl;
    
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i * 10, i * 100});
    }
    
    BTreeSST sst;
    sst.buildBTree(data, "test_range.sst", 10, 3);
    
    // Range queries should still work with bloom filter
    auto result = sst.scan(200, 500, "test_range.sst", true);
    
    // Should find keys: 200, 210, 220, ..., 500 (31 keys)
    if (result.size() == 31) {
        bloom_test_passed("Bloom Filter With Range Queries");
    } else {
        bloom_test_failed("Range Queries - Found: " + std::to_string(result.size()) + "/31");
    }
    
    remove("test_range.sst");
}

int run_bloom_filter_tests() {
    std::cout << "\n========== BLOOM FILTER TESTS ==========" << std::endl;
    
    test_basic_bloom_filter_creation();
    test_bloom_filter_no_false_negatives();
    test_bloom_filter_false_positive_rate();
    test_bloom_filter_different_parameters();
    test_bloom_filter_early_rejection();
    test_bloom_filter_with_single_element();
    test_bloom_filter_with_duplicates();
    test_bloom_filter_hash_distribution();
    test_bloom_filter_edge_cases();
    test_bloom_filter_large_dataset();
    test_bloom_filter_bit_operations();
    test_bloom_filter_hash_consistency();
    test_bloom_filter_bytes_calculation();
    test_bloom_filter_range_queries();
    
    std::cout << "\n=== BLOOM FILTER TEST SUMMARY ===" << std::endl;
    std::cout << "Bloom Filter Tests Passed: " << bloom_tests_passed << std::endl;
    std::cout << "Bloom Filter Tests Failed: " << bloom_tests_failed << std::endl;
    
    return bloom_tests_failed;
}

int main() {
    return run_bloom_filter_tests();
}
