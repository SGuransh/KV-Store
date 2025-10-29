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

# Test sources
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)

# Test executables
TESTS = $(patsubst $(TEST_DIR)/%.cpp,$(OUT_DIR)/%,$(TEST_SOURCES))

# Default target
all: $(TESTS) test main

# Main CLI application
main: main.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Rule for building test executables directly from test + sources
$(OUT_DIR)/%: $(TEST_DIR)/%.cpp $(SOURCES)
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) $< $(SOURCES) -o $@

# Main test runner
test: test.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@./test

# Run all test executables individually
test-individual: $(TESTS)
	@echo "========================================="
	@echo "         Running All Tests"
	@echo "========================================="
	@for t in $(TESTS); do ./$$t; done
	@echo "========================================="
	@echo "         All Tests Completed"
	@echo "========================================="

# Clean up
clean:
	rm -rf $(OUT_DIR) test main
	rm -rf test_db_* test_memtable_dir
	rm -rf $(C_DIR)
	rm -rf test.exe
	rm -rf *.o

.PHONY: all test clean test-individual main