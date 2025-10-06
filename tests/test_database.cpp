#include "../FileOperations.hpp"
#include "../Database.hpp"
#include <iostream>

int db_tests_passed = 0;
int db_tests_failed = 0;

void db_test_passed(const std::string& test_name) {
    db_tests_passed++;
    std::cout << "[DB] " << test_name << " PASSED!" << std::endl;
}

void db_test_failed(const std::string& test_name) {
    db_tests_failed++;
    std::cout << "[DB] " << test_name << " FAILED!" << std::endl;
}

void cleanup_test_directory(const std::string& dirName) {
    std::string command = "rm -rf " + dirName;
    system(command.c_str());
}

int run_database_tests() {
    std::cout << "\n========== DATABASE TESTS ==========" << std::endl;
    
    cleanup_test_directory("test_db_simple");
    
    Database db(5);
    
    if (db.open_database("test_db_simple")) {
        db_test_passed("Database Open");
    } else {
        db_test_failed("Database Open");
        return 1;
    }
    
    if (db.insert(10, 100) && db.insert(20, 200)) {
        db_test_passed("Database Insert");
    } else {
        db_test_failed("Database Insert");
    }
    
    int value;
    if (db.search(10, value) && value == 100) {
        db_test_passed("Database Search");
    } else {
        db_test_failed("Database Search");
    }
    
    if (db.close_database()) {
        db_test_passed("Database Close");
    } else {
        db_test_failed("Database Close");
    }
    
    cleanup_test_directory("test_db_simple");
    
    std::cout << "\n=== DATABASE TEST SUMMARY ===" << std::endl;
    std::cout << "Database Tests Passed: " << db_tests_passed << std::endl;
    std::cout << "Database Tests Failed: " << db_tests_failed << std::endl;
    
    return db_tests_failed;
}

int main() {
    return run_database_tests();
}