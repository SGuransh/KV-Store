# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall

# Directories
SRC_DIR = .
TEST_DIR = tests
OUT_DIR = out
C_DIR = cFiles

# Source files
SOURCES = $(SRC_DIR)/AVL.cpp $(SRC_DIR)/FileOperations.cpp $(SRC_DIR)/Database.cpp $(SRC_DIR)/PageID.cpp $(SRC_DIR)/Page.cpp $(SRC_DIR)/MurmurHash.cpp $(SRC_DIR)/BucketNode.cpp $(SRC_DIR)/Bucket.cpp $(SRC_DIR)/HashTable.cpp $(SRC_DIR)/BufferPool.cpp $(SRC_DIR)/ClockEvictionPolicy.cpp
SRC = AVL.cpp FileOperations.cpp Database.cpp PageID.cpp Page.cpp MurmurHash.cpp BucketNode.cpp Bucket.cpp HashTable.cpp BufferPool.cpp ClockEvictionPolicy.cpp
HEADERS = Memtable_ds.hpp FileOperations.hpp Database.hpp AVL.hpp PageID.hpp Page.hpp MurmurHash.hpp BucketNode.hpp Bucket.hpp HashTable.hpp BufferPool.hpp ClockEvictionPolicy.hpp EvictionPolicy.hpp

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