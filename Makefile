# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall

# Directories
SRC_DIR = .
TEST_DIR = tests
OUT_DIR = out
C_DIR = cFiles

# Source files
SOURCES = $(SRC_DIR)/AVL.cpp $(SRC_DIR)/FileOperations.cpp $(SRC_DIR)/Database.cpp
SRC = AVL.cpp FileOperations.cpp Database.cpp
HEADERS = Memtable_ds.hpp FileOperations.hpp 

# Test sources
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)

# Test executables
TESTS = $(patsubst $(TEST_DIR)/%.cpp,$(OUT_DIR)/%,$(TEST_SOURCES))

# Default target
all: $(TESTS) test

# Rule for building test executables directly from test + sources
$(OUT_DIR)/%: $(TEST_DIR)/%.cpp 
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Main test runner
test: test.cpp
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
	rm -rf $(OUT_DIR) test
	rm -rf test_db_* test_memtable_dir
	rm -rf $(C_DIR)

.PHONY: all test clean test-individual

