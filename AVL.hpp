#pragma once
#include "Memtable_ds.hpp"
#include <vector>
#include <memory>
#include <string>

class AVL : public Memtable_ds {
private:
    int currentSize = 0;
    int nextFileNumber = 1;
    std::string databaseDirectory;

public:
    AVL(int maxElements);
    AVL(std::vector<std::pair<int,int>> sst, int maxElements);

    Node* insert(int key, int value) override;
    bool search(int key, int& value) override;
    Node* remove(int key) override;
    int get_size() const override;
    int get_max_elements() const override;
    std::vector<int> inorder() override;

    Node* timed_insert(int key, int value, int& time) override;
    bool timed_search(int key, int& value, int& time) override;
    Node* timed_remove(int key, int& time) override;

    std::vector<std::pair<int,int>> range_scan(int key1, int key2) override;
    bool flush_to_sst(int fileNumber, bool isComplete = true) override;
    bool load_from_sst(const std::vector<std::pair<int,int>>& data) override;
    void set_next_file_number(int nextFileNum) override;
    void set_database_directory(const std::string& dbDir) override;

    // Additional methods for SST file operations
    bool get_from_sst(int key, int& value);
    std::vector<std::pair<int, int>> range_scan_sst_files(int key1, int key2);
    bool binary_search_sst(const std::vector<std::pair<int, int>>& pairs, int key, int& value);
    int get_next_file_number() const override;
};
