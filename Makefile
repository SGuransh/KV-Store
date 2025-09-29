# Compiler
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Source files  
SOURCES = AVL.cpp FileOperations.cpp Database.cpp
HEADERS = Memtable_ds.hpp FileOperations.hpp

# Test executables
TESTS = test_avl test_memtable test_database

# Default target
all: $(TESTS)

# Individual test targets
test_avl: test_avl.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ test_avl.cpp

test_memtable: test_memtable.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ test_memtable.cpp

test_database: test_database.cpp $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ test_database.cpp

# Main test runner
test: $(TESTS) test.cpp
	$(CXX) $(CXXFLAGS) -o test test.cpp
	@./test

# Run individual tests
test-individual: $(TESTS)
	@echo "========================================="
	@echo "         Running All Tests"
	@echo "========================================="
	@./test_avl
	@./test_memtable  
	@./test_database
	@echo "========================================="
	@echo "         All Tests Completed"
	@echo "========================================="

# Clean up
clean:
	rm -f $(TESTS) test
	rm -rf test_db_* test_memtable_dir

.PHONY: all test clean