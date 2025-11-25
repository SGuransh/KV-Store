#include "../FileOperations.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstdlib>

int tests_passed = 0;
int tests_failed = 0;

void test_passed(const std::string& test_name) {
    tests_passed++;
    std::cout << "✓ " << test_name << " PASSED" << std::endl;
}

void test_failed(const std::string& test_name, const std::string& reason = "") {
    tests_failed++;
    std::cout << "✗ " << test_name << " FAILED";
    if (!reason.empty()) {
        std::cout << ": " << reason;
    }
    std::cout << std::endl;
}

void cleanup_test_directory(const std::string& dirName) {
    std::string command = "rm -rf " + dirName;
    system(command.c_str());
}

void test_create_directory() {
    std::cout << "\n[Test 1: Create Directory]" << std::endl;
    
    std::string testDir = "test_dir_create";
    cleanup_test_directory(testDir);
    
    // Test creating a new directory
    if (FileOperations::create_directory(testDir)) {
        if (FileOperations::directory_exists(testDir)) {
            test_passed("Create new directory");
        } else {
            test_failed("Create new directory", "Directory not found after creation");
        }
    } else {
        test_failed("Create new directory", "create_directory returned false");
    }
    
    // Test creating directory that already exists (should succeed)
    if (FileOperations::create_directory(testDir)) {
        test_passed("Create existing directory (idempotent)");
    } else {
        test_failed("Create existing directory", "Should succeed for existing directory");
    }
    
    cleanup_test_directory(testDir);
}

void test_directory_exists() {
    std::cout << "\n[Test 2: Directory Exists]" << std::endl;
    
    std::string testDir = "test_dir_exists";
    cleanup_test_directory(testDir);
    
    // Test non-existent directory
    if (!FileOperations::directory_exists(testDir)) {
        test_passed("Non-existent directory returns false");
    } else {
        test_failed("Non-existent directory", "Should return false");
    }
    
    // Create directory and test again
    FileOperations::create_directory(testDir);
    if (FileOperations::directory_exists(testDir)) {
        test_passed("Existing directory returns true");
    } else {
        test_failed("Existing directory", "Should return true");
    }
    
    cleanup_test_directory(testDir);
}

void test_file_exists() {
    std::cout << "\n[Test 3: File Exists]" << std::endl;
    
    std::string testDir = "test_file_exists_dir";
    std::string testFile = testDir + "/test.sst";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    // Test non-existent file
    if (!FileOperations::file_exists(testFile)) {
        test_passed("Non-existent file returns false");
    } else {
        test_failed("Non-existent file", "Should return false");
    }
    
    // Create file
    std::vector<std::pair<int, int>> data = {{1, 10}, {2, 20}};
    FileOperations::write_sst_file(data, testFile, false);
    
    // Test existing file
    if (FileOperations::file_exists(testFile)) {
        test_passed("Existing file returns true");
    } else {
        test_failed("Existing file", "Should return true");
    }
    
    cleanup_test_directory(testDir);
}

void test_write_and_read_sst() {
    std::cout << "\n[Test 4: Write and Read SST File]" << std::endl;
    
    std::string testDir = "test_write_read";
    std::string testFile = testDir + "/data.sst";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    // Test writing empty file
    std::vector<std::pair<int, int>> emptyData;
    if (FileOperations::write_sst_file(emptyData, testFile, false)) {
        std::vector<std::pair<int, int>> readData = FileOperations::read_sst_file(testFile);
        if (readData.empty()) {
            test_passed("Write and read empty SST file");
        } else {
            test_failed("Write and read empty SST", "Expected empty, got data");
        }
    } else {
        test_failed("Write empty SST", "write_sst_file returned false");
    }
    
    // Test writing single pair
    std::vector<std::pair<int, int>> singleData = {{42, 100}};
    if (FileOperations::write_sst_file(singleData, testFile, false)) {
        std::vector<std::pair<int, int>> readData = FileOperations::read_sst_file(testFile);
        if (readData.size() == 1 && readData[0].first == 42 && readData[0].second == 100) {
            test_passed("Write and read single pair SST file");
        } else {
            test_failed("Write and read single pair", "Data mismatch");
        }
    } else {
        test_failed("Write single pair SST", "write_sst_file returned false");
    }
    
    // Test writing multiple pairs
    std::vector<std::pair<int, int>> multiData = {
        {10, 100}, {20, 200}, {30, 300}, {40, 400}, {50, 500}
    };
    if (FileOperations::write_sst_file(multiData, testFile, false)) {
        std::vector<std::pair<int, int>> readData = FileOperations::read_sst_file(testFile);
        bool match = readData.size() == multiData.size();
        if (match) {
            for (size_t i = 0; i < readData.size(); ++i) {
                if (readData[i].first != multiData[i].first || 
                    readData[i].second != multiData[i].second) {
                    match = false;
                    break;
                }
            }
        }
        if (match) {
            test_passed("Write and read multiple pairs SST file");
        } else {
            test_failed("Write and read multiple pairs", "Data mismatch");
        }
    } else {
        test_failed("Write multiple pairs SST", "write_sst_file returned false");
    }
    
    cleanup_test_directory(testDir);
}

void test_atomic_write() {
    std::cout << "\n[Test 5: Atomic Write]" << std::endl;
    
    std::string testDir = "test_atomic";
    std::string testFile = testDir + "/atomic.sst";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    // Test atomic write (default behavior)
    std::vector<std::pair<int, int>> data = {{1, 10}, {2, 20}, {3, 30}};
    if (FileOperations::write_sst_file(data, testFile, true)) {
        // Check that temp file doesn't exist
        std::string tempFile = testFile + ".tmp";
        if (!FileOperations::file_exists(tempFile)) {
            test_passed("Atomic write (no temp file left behind)");
        } else {
            test_failed("Atomic write", "Temp file still exists");
        }
        
        // Verify data is correct
        std::vector<std::pair<int, int>> readData = FileOperations::read_sst_file(testFile);
        if (readData == data) {
            test_passed("Atomic write data integrity");
        } else {
            test_failed("Atomic write data integrity", "Data mismatch");
        }
    } else {
        test_failed("Atomic write", "write_sst_file returned false");
    }
    
    cleanup_test_directory(testDir);
}

void test_count_sst_files() {
    std::cout << "\n[Test 6: Count SST Files]" << std::endl;
    
    std::string testDir = "test_count_sst";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    // Test empty directory
    int count = FileOperations::count_sst_files(testDir);
    if (count == 0) {
        test_passed("Count SST files in empty directory");
    } else {
        test_failed("Count SST files in empty directory", "Expected 0, got " + std::to_string(count));
    }
    
    // Create SST files with proper naming convention (number.txt)
    std::vector<std::pair<int, int>> data = {{1, 10}};
    FileOperations::write_sst_file(data, testDir + "/1.txt", false);
    FileOperations::write_sst_file(data, testDir + "/2.txt", false);
    FileOperations::write_sst_file(data, testDir + "/3.txt", false);
    
    // Create non-SST files (should not be counted)
    FileOperations::write_sst_file(data, testDir + "/incomplete.txt", false);
    FileOperations::write_sst_file(data, testDir + "/data.sst", false);
    FileOperations::write_sst_file(data, testDir + "/notanumber.txt", false);
    
    count = FileOperations::count_sst_files(testDir);
    if (count == 3) {
        test_passed("Count SST files with mixed files");
    } else {
        test_failed("Count SST files", "Expected 3, got " + std::to_string(count));
    }
    
    cleanup_test_directory(testDir);
}

void test_remove_file() {
    std::cout << "\n[Test 7: Remove File]" << std::endl;
    
    std::string testDir = "test_remove";
    std::string testFile = testDir + "/remove_me.sst";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    // Create a file
    std::vector<std::pair<int, int>> data = {{1, 10}};
    FileOperations::write_sst_file(data, testFile, false);
    
    // Verify file exists
    if (!FileOperations::file_exists(testFile)) {
        test_failed("Remove file setup", "File was not created");
        cleanup_test_directory(testDir);
        return;
    }
    
    // Remove file
    if (FileOperations::remove_file(testFile)) {
        if (!FileOperations::file_exists(testFile)) {
            test_passed("Remove existing file");
        } else {
            test_failed("Remove file", "File still exists after removal");
        }
    } else {
        test_failed("Remove file", "remove_file returned false");
    }
    
    // Try to remove non-existent file (should return false)
    if (!FileOperations::remove_file(testFile)) {
        test_passed("Remove non-existent file returns false");
    } else {
        test_failed("Remove non-existent file", "Should return false");
    }
    
    cleanup_test_directory(testDir);
}

void test_large_sst_file() {
    std::cout << "\n[Test 8: Large SST File I/O]" << std::endl;
    
    std::string testDir = "test_large_sst";
    std::string testFile = testDir + "/large.sst";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    // Create large dataset (10,000 pairs)
    std::vector<std::pair<int, int>> largeData;
    for (int i = 0; i < 10000; ++i) {
        largeData.push_back({i, i * 10});
    }
    
    // Write large file
    if (FileOperations::write_sst_file(largeData, testFile, false)) {
        test_passed("Write large SST file (10,000 pairs)");
        
        // Read and verify
        std::vector<std::pair<int, int>> readData = FileOperations::read_sst_file(testFile);
        if (readData == largeData) {
            test_passed("Read and verify large SST file");
        } else {
            test_failed("Read large SST file", "Data mismatch or size mismatch");
        }
    } else {
        test_failed("Write large SST file", "write_sst_file returned false");
    }
    
    cleanup_test_directory(testDir);
}

void test_edge_cases() {
    std::cout << "\n[Test 9: Edge Cases]" << std::endl;
    
    // Test reading non-existent file
    std::vector<std::pair<int, int>> data = FileOperations::read_sst_file("nonexistent.sst");
    if (data.empty()) {
        test_passed("Read non-existent file returns empty vector");
    } else {
        test_failed("Read non-existent file", "Should return empty vector");
    }
    
    // Test count on non-existent directory
    int count = FileOperations::count_sst_files("nonexistent_directory");
    if (count == 0) {
        test_passed("Count SST files in non-existent directory");
    } else {
        test_failed("Count non-existent directory", "Expected 0, got " + std::to_string(count));
    }
    
    // Test with negative and zero values
    std::string testDir = "test_edge";
    std::string testFile = testDir + "/edge.sst";
    cleanup_test_directory(testDir);
    FileOperations::create_directory(testDir);
    
    std::vector<std::pair<int, int>> edgeData = {
        {-1000, -5000}, {-1, -1}, {0, 0}, {1, 1}, {1000, 5000}
    };
    FileOperations::write_sst_file(edgeData, testFile, false);
    std::vector<std::pair<int, int>> readData = FileOperations::read_sst_file(testFile);
    
    if (readData == edgeData) {
        test_passed("Handle negative and zero values");
    } else {
        test_failed("Handle negative values", "Data mismatch");
    }
    
    cleanup_test_directory(testDir);
}

int main() {
    std::cout << "╔═══════════════════════════════════════╗" << std::endl;
    std::cout << "║  FileOperations Tests                 ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════╝" << std::endl;
    
    try {
        test_create_directory();
        test_directory_exists();
        test_file_exists();
        test_write_and_read_sst();
        test_atomic_write();
        test_count_sst_files();
        test_remove_file();
        test_large_sst_file();
        test_edge_cases();
        
        std::cout << "\n╔═══════════════════════════════════════╗" << std::endl;
        std::cout << "║  Test Summary                         ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════╝" << std::endl;
        std::cout << "  Tests Passed: " << tests_passed << std::endl;
        std::cout << "  Tests Failed: " << tests_failed << std::endl;
        
        if (tests_failed == 0) {
            std::cout << "\n🎉 All FileOperations tests PASSED! 🎉" << std::endl;
            return 0;
        } else {
            std::cout << "\n❌ Some tests FAILED ❌" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cout << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
