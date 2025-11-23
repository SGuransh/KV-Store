#pragma once

#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "FileOperations.hpp"
#include "AVL.hpp" 
#include <string>
#include <vector>
#include "Memtable_ds.hpp"
#include "LSM/LSMTree.hpp"
#include <memory>

class Database {
private:
    std::unique_ptr<Memtable_ds> engine;
    std::unique_ptr<LSMTree> lsmTree;
    std::string databaseName;
    std::string databaseDirectory;
    bool isOpen;
    int nextFileNumber;

    bool load_incomplete_file();

public:
    Database(int memtableCapacity = 1000);
    ~Database();

    bool open_database(const std::string& dbName);
    bool close_database();

    bool insert(int key, int value);
    bool search(int key, int& value);
    std::vector<std::pair<int, int>> range_scan(int key1, int key2);
    
    // LSM operations
    void print_lsm_structure() const;
    bool compact_level(int level);

    // Getters
    int get_size() const;
    int get_max_elements() const;
    bool is_open() const;
    std::string get_database_name() const;
};

#endif // DATABASE_HPP