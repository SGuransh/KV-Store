# KV-Store

# Compile AVL.cpp as object file

g++ -std=c++17 -c AVL.cpp -o AVL.o

# Compile and run test suite

g++ -std=c++17 -o test test.cpp
./test

# Using Make File

make test # Run all tests
make test_avl # Run only AVL tests  
make test_database # Run only database tests
make clean # Clean up build artifacts
