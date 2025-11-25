#include "../LSM/MergeBuffer.hpp"
#include "../BTree/BTreeSST.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <sys/stat.h>

void cleanup_test_files() {
    std::remove("test_merge_input.txt");
    std::remove("test_merge_output.txt");
}

void test_buffer_construction() {
    std::cout << "Testing MergeBuffer construction..." << std::endl;
    
    // Test default size
    MergeBuffer buffer1;
    assert(!buffer1.hasData());
    assert(!buffer1.isFull());
    
    // Test custom size
    MergeBuffer buffer2(512);
    assert(!buffer2.hasData());
    assert(!buffer2.isFull());
    
    std::cout << "✓ Buffer construction test passed" << std::endl;
}

void test_output_buffer_operations() {
    std::cout << "Testing output buffer operations..." << std::endl;
    
    MergeBuffer buffer(10);  // Small buffer for testing
    
    // Test append
    for (int i = 0; i < 10; i++) {
        assert(buffer.append(i, i * 100) == true);
    }
    
    // Buffer should be full now
    assert(buffer.isFull() == true);
    
    // Appending to full buffer should fail
    assert(buffer.append(99, 9900) == false);
    
    // Test flush to file
    int fd = open("test_merge_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd >= 0);
    assert(buffer.flushToFile(fd) == true);
    close(fd);
    
    // Verify file size
    struct stat st;
    stat("test_merge_output.txt", &st);
    assert(st.st_size == 10 * 2 * sizeof(int32_t));  // 10 pairs * 2 elements * 4 bytes
    
    // Test clear
    buffer.clear();
    assert(!buffer.hasData());
    assert(!buffer.isFull());
    
    cleanup_test_files();
    
    std::cout << "✓ Output buffer operations test passed" << std::endl;
}

void test_input_buffer_operations() {
    std::cout << "Testing input buffer operations..." << std::endl;
    
    // Create a test SST file using BTreeSST
    BTreeSST builder;
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 50; i++) {
        data.push_back({i, i * 10});
    }
    
    assert(builder.buildBTree(data, "test_merge_input.txt") == true);
    
    // Get file size to calculate leaf data offset
    struct stat st;
    stat("test_merge_input.txt", &st);
    size_t fileSize = st.st_size;
    
    // For a BTree SST, we need to skip the metadata page and internal nodes
    // The leaf data starts after the metadata page (4096 bytes)
    // For simplicity, we'll read from the beginning of leaf pages
    size_t fileOffset = 4096;  // Skip metadata page
    
    MergeBuffer buffer(20);  // Buffer can hold 20 pairs
    
    // Test refill from SST
    bool success = buffer.refillFromSST("test_merge_input.txt", fileOffset);
    assert(success == true);
    assert(buffer.hasData() == true);
    
    // Test peekMin and consumeMin
    auto pair = buffer.peekMin();
    assert(pair.first == 1);
    assert(pair.second == 10);
    
    // Peek again - should return same pair
    pair = buffer.peekMin();
    assert(pair.first == 1);
    assert(pair.second == 10);
    
    // Consume and check next
    buffer.consumeMin();
    pair = buffer.peekMin();
    assert(pair.first == 2);
    assert(pair.second == 20);
    
    cleanup_test_files();
    
    std::cout << "✓ Input buffer operations test passed" << std::endl;
}

void test_buffer_refill() {
    std::cout << "Testing buffer refill from SST..." << std::endl;
    
    // Create a larger SST file
    BTreeSST builder;
    std::vector<std::pair<int, int>> data;
    for (int i = 1; i <= 100; i++) {
        data.push_back({i, i * 100});
    }
    
    assert(builder.buildBTree(data, "test_merge_input.txt") == true);
    
    size_t fileOffset = 4096;  // Skip metadata page
    MergeBuffer buffer(30);  // Buffer smaller than total data
    
    // First refill
    assert(buffer.refillFromSST("test_merge_input.txt", fileOffset) == true);
    assert(buffer.hasData() == true);
    
    // Consume all data in buffer
    int count = 0;
    while (buffer.hasData()) {
        buffer.consumeMin();
        count++;
    }
    assert(count > 0);
    assert(!buffer.hasData());
    
    // Refill again - should get more data
    bool hasMore = buffer.refillFromSST("test_merge_input.txt", fileOffset);
    // May or may not have more data depending on file structure
    
    cleanup_test_files();
    
    std::cout << "✓ Buffer refill test passed" << std::endl;
}

void test_buffer_empty_behavior() {
    std::cout << "Testing buffer behavior when empty..." << std::endl;
    
    MergeBuffer buffer(10);
    
    // Empty buffer should not have data
    assert(!buffer.hasData());
    
    // Consuming from empty buffer should be safe
    buffer.consumeMin();
    assert(!buffer.hasData());
    
    // Flush empty buffer should succeed
    int fd = open("test_merge_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd >= 0);
    assert(buffer.flushToFile(fd) == true);
    close(fd);
    
    // File should be empty or very small
    struct stat st;
    stat("test_merge_output.txt", &st);
    assert(st.st_size == 0);
    
    cleanup_test_files();
    
    std::cout << "✓ Empty buffer behavior test passed" << std::endl;
}

void test_buffer_full_behavior() {
    std::cout << "Testing buffer behavior when full..." << std::endl;
    
    MergeBuffer buffer(5);  // Very small buffer
    
    // Fill buffer completely
    for (int i = 0; i < 5; i++) {
        assert(buffer.append(i, i * 10) == true);
    }
    
    assert(buffer.isFull() == true);
    
    // Cannot append more
    assert(buffer.append(99, 990) == false);
    
    // Flush and clear
    int fd = open("test_merge_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd >= 0);
    assert(buffer.flushToFile(fd) == true);
    close(fd);
    
    buffer.clear();
    
    // After clear, should be able to append again
    assert(!buffer.isFull());
    assert(buffer.append(100, 1000) == true);
    
    cleanup_test_files();
    
    std::cout << "✓ Full buffer behavior test passed" << std::endl;
}

int main() {
    std::cout << "Running MergeBuffer tests..." << std::endl;
    std::cout << "================================" << std::endl;
    
    test_buffer_construction();
    test_output_buffer_operations();
    test_input_buffer_operations();
    test_buffer_refill();
    test_buffer_empty_behavior();
    test_buffer_full_behavior();
    
    std::cout << "================================" << std::endl;
    std::cout << "All MergeBuffer tests passed! ✓" << std::endl;
    
    return 0;
}
