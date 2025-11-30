# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall

# Directories
SRC_DIR = .
TEST_DIR = tests
OUT_DIR = out
C_DIR = cFiles

# Source files
# Core and database sources
SOURCES = $(SRC_DIR)/AVL.cpp \
	    $(SRC_DIR)/FileOperations.cpp \
	    $(SRC_DIR)/Database.cpp \
	    $(SRC_DIR)/LSM/LSMTree.cpp \
	    $(SRC_DIR)/LSM/MergeBuffer.cpp \
	    $(SRC_DIR)/BTree/BTreeSST.cpp \
	    $(SRC_DIR)/BufferPool/PageID.cpp \
	    $(SRC_DIR)/BufferPool/Page.cpp \
	    $(SRC_DIR)/BufferPool/MurmurHash.cpp \
	    $(SRC_DIR)/BufferPool/BucketNode.cpp \
	    $(SRC_DIR)/BufferPool/Bucket.cpp \
	    $(SRC_DIR)/BufferPool/HashTable.cpp \
	    $(SRC_DIR)/BufferPool/BufferPool.cpp \
	    $(SRC_DIR)/BufferPool/ClockEvictionPolicy.cpp

# Convenience (unused directly but kept for reference)
SRC = AVL.cpp FileOperations.cpp Database.cpp \
	BufferPool/PageID.cpp BufferPool/Page.cpp BufferPool/MurmurHash.cpp \
	BufferPool/BucketNode.cpp BufferPool/Bucket.cpp BufferPool/HashTable.cpp \
	BufferPool/BufferPool.cpp BufferPool/ClockEvictionPolicy.cpp

# Header list (for reference/tools)
HEADERS = Memtable_ds.hpp FileOperations.hpp Database.hpp AVL.hpp \
	    BufferPool/PageID.hpp BufferPool/Page.hpp BufferPool/MurmurHash.hpp \
	    BufferPool/BucketNode.hpp BufferPool/Bucket.hpp BufferPool/HashTable.hpp \
	    BufferPool/BufferPool.hpp BufferPool/ClockEvictionPolicy.hpp BufferPool/EvictionPolicy.hpp

# B-Tree source files
BTREE_SOURCES = BTree/BTreeSST.cpp BufferPool/Page.cpp

# LSM source files (includes BTree dependencies)
LSM_SOURCES = LSM/LSMTree.cpp LSM/MergeBuffer.cpp BTree/BTreeSST.cpp BufferPool/Page.cpp FileOperations.cpp

# Test sources
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)

# Test executables
TESTS = $(patsubst $(TEST_DIR)/%.cpp,$(OUT_DIR)/%,$(TEST_SOURCES))

# Default target - runs everything
all: $(TESTS) test main test-btree-auto test-file-operations

# Main CLI application
main: main.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@

# BTree read with setup test (needs both BTree and LSM) - must come before pattern rule
$(OUT_DIR)/test_btree_read_with_setup: $(TEST_DIR)/test_btree_read_with_setup.cpp $(LSM_SOURCES)
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

# Rule for building test executables directly from test + sources
$(OUT_DIR)/%: $(TEST_DIR)/%.cpp $(SOURCES)
	@mkdir -p $(OUT_DIR)
	@if [[ $* == *btree* ]] || [[ $* == *bloom* ]]; then \
		$(CXX) $(CXXFLAGS) -I. $< $(BTREE_SOURCES) -o $@; \
	elif [[ $* == *merge* ]] || [[ $* == *compaction* ]] || [[ $* == *lsm* ]] || [[ $* == *sstmetadata* ]]; then \
		$(CXX) $(CXXFLAGS) -I. $< $(LSM_SOURCES) -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $< $(SOURCES) -o $@; \
	fi

# Main test runner
test: test.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@./test

# Run B-Tree tests as part of default build
test-btree-auto: test_btree_get test_btree_internal_levels test_btree_scan test_bloom_filter
	@echo ""
	@echo "========================================="
	@echo "       Running B-Tree Tests"
	@echo "========================================="
	@echo "Running B-Tree Search Tests..."
	@./test_btree_get
	@echo ""
	@echo "Running B-Tree Internal Levels Tests..."
	@./test_btree_internal_levels
	@echo ""
	@echo "Running B-Tree Scan Tests..."
	@./test_btree_scan
	@echo ""
	@echo "Running Bloom Filter Tests..."
	@./test_bloom_filter
	@echo "========================================="
	@echo "       B-Tree Tests Completed"
	@echo "========================================="

# Run all test executables individually
test-individual: $(TESTS)
	@echo "========================================="
	@echo "         Running All Tests"
	@echo "========================================="
	@for t in $(TESTS); do ./$$t; done
	@echo "========================================="
	@echo "         All Tests Completed"
	@echo "========================================="

# B-Tree specific test targets
test_btree_get: tests/test_btree_get.cpp $(BTREE_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

test_btree_internal_levels: tests/test_btree_internal_levels.cpp $(BTREE_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

test_btree_scan: tests/test_btree_scan.cpp $(BTREE_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

test_bloom_filter: tests/test_bloom_filter.cpp $(BTREE_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

# FileOperations test target
test_file_operations: tests/test_file_operations.cpp FileOperations.cpp $(BTREE_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

# LSM specific test targets
test_merge_algorithm: tests/test_merge_algorithm.cpp $(LSM_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

test_mergebuffer: tests/test_mergebuffer.cpp $(LSM_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

test_compaction: tests/test_compaction.cpp $(LSM_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

test_sstmetadata: tests/test_sstmetadata.cpp $(LSM_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

# Run B-Tree tests
test-btree: test_btree_get test_btree_internal_levels test_btree_scan test_bloom_filter
	@echo "========================================="
	@echo "       Running B-Tree Tests"
	@echo "========================================="
	@echo "Running B-Tree Search Tests..."
	@./test_btree_get
	@echo ""
	@echo "Running B-Tree Internal Levels Tests..."
	@./test_btree_internal_levels
	@echo ""
	@echo "Running B-Tree Scan Tests..."
	@./test_btree_scan
	@echo ""
	@echo "Running Bloom Filter Tests..."
	@./test_bloom_filter
	@echo "========================================="
	@echo "       B-Tree Tests Completed"
	@echo "========================================="

# Run all tests including B-Tree tests
test-all: test test-btree
	@echo "========================================="
	@echo "      All Tests Completed Successfully"
	@echo "========================================="

# Run FileOperations tests
test-file-operations: test_file_operations
	@echo "========================================="
	@echo "    Running FileOperations Tests"
	@echo "========================================="
	@./test_file_operations
	@echo "========================================="
	@echo "  FileOperations Tests Completed"
	@echo "========================================="

# Run LSM tests
test-lsm: test_merge_algorithm test_mergebuffer test_compaction test_sstmetadata
	@echo "========================================="
	@echo "       Running LSM Tests"
	@echo "========================================="
	@echo "Running Merge Algorithm Tests..."
	@./test_merge_algorithm
	@echo ""
	@echo "Running MergeBuffer Tests..."
	@./test_mergebuffer
	@echo ""
	@echo "Running Compaction Tests..."
	@./test_compaction
	@echo ""
	@echo "Running SST Metadata Tests..."
	@./test_sstmetadata
	@echo "========================================="
	@echo "       LSM Tests Completed"
	@echo "========================================="

# Clean up
clean:
	rm -rf $(OUT_DIR) test main
	rm -rf test_db_* test_memtable_dir
	rm -rf $(C_DIR)
	rm -rf test.exe
	rm -rf *.o
	rm -rf test_btree_get test_btree_internal_levels test_btree_scan test_bloom_filter test_file_operations
	rm -rf test_merge_algorithm test_mergebuffer test_compaction test_sstmetadata
	rm -rf /tmp/btree_*
	rm -rf test_dir_* test_file_* test_write_* test_atomic test_count_sst test_remove test_large_sst test_edge
	rm -rf test_merge_* test_compaction_* test_cascade_* test_get_* test_scan_* test_update_* test_recovery_*
	rm -rf *.sst

.PHONY: all test clean test-individual main test-btree test-all test-file-operations test-lsm