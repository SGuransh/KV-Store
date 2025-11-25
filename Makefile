# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall

# Directories
SRC_DIR = .
TEST_DIR = tests
OUT_DIR = out
C_DIR = cFiles

# Source files
SOURCES = $(SRC_DIR)/AVL.cpp $(SRC_DIR)/FileOperations.cpp $(SRC_DIR)/Database.cpp $(BTREE_SOURCES)
SRC = AVL.cpp FileOperations.cpp Database.cpp
HEADERS = Memtable_ds.hpp FileOperations.hpp Database.hpp AVL.hpp

# B-Tree source files
BTREE_SOURCES = BTree/BTreeSST.cpp

# Test sources
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)

# Test executables
TESTS = $(patsubst $(TEST_DIR)/%.cpp,$(OUT_DIR)/%,$(TEST_SOURCES))

# Default target - runs everything
all: $(TESTS) test main test-btree-auto test-file-operations

# Main CLI application
main: main.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Rule for building test executables directly from test + sources
$(OUT_DIR)/%: $(TEST_DIR)/%.cpp $(SOURCES)
	@mkdir -p $(OUT_DIR)
	@if [[ $* == *btree* ]]; then \
		$(CXX) $(CXXFLAGS) -I. $< $(BTREE_SOURCES) -o $@; \
	else \
		$(CXX) $(CXXFLAGS) $< $(SOURCES) -o $@; \
	fi

# Main test runner
test: test.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@
	@./test

# Run B-Tree tests as part of default build
test-btree-auto: test_btree_get test_btree_internal_levels test_btree_scan
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

# FileOperations test target
test_file_operations: tests/test_file_operations.cpp FileOperations.cpp $(BTREE_SOURCES)
	$(CXX) $(CXXFLAGS) -I. $^ -o $@

# Run B-Tree tests
test-btree: test_btree_get test_btree_internal_levels test_btree_scan
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

# Search Order test target
test_search_order: test_search_order.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Run Search Order test
test-search-order: test_search_order
	@echo "========================================="
	@echo "     Running Search Order Test"
	@echo "========================================="
	@./test_search_order
	@echo "========================================="
	@echo "   Search Order Test Completed"
	@echo "========================================="

# User Scenario test target
test_user_scenario: test_user_scenario.cpp $(SOURCES)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Run User Scenario test
test-user-scenario: test_user_scenario
	@echo "========================================="
	@echo "     Running User Scenario Test"
	@echo "========================================="
	@./test_user_scenario
	@echo "========================================="
	@echo "   User Scenario Test Completed"
	@echo "========================================="

# Clean up
clean:
	rm -rf $(OUT_DIR) test main
	rm -rf test_db_* test_memtable_dir
	rm -rf $(C_DIR)
	rm -rf test.exe
	rm -rf *.o
	rm -rf test_btree_get test_btree_internal_levels test_btree_scan test_file_operations
	rm -rf test_search_order test_user_scenario
	rm -rf /tmp/btree_*
	rm -rf test_dir_* test_file_* test_write_* test_atomic test_count_sst test_remove test_large_sst test_edge
	rm -rf user_test

.PHONY: all test clean test-individual main test-btree test-all test-file-operations