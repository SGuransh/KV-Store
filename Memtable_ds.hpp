#include <vector>
#include <utility>
#include <string>

#ifndef MEMTABLE_DS_HPP
#define MEMTABLE_DS_HPP

class Memtable_ds {
protected:
    struct Node {
        int key;
        int value;
        Node* left;
        Node* right;
        int height;
    };

    int maxElements;
    Node* root; 

public:
    Memtable_ds(int maxElements) : maxElements(maxElements) {}
    Memtable_ds(std::vector<std::pair<int, int>> sst, int maxElements) : maxElements(maxElements) {}

    virtual Node* insert(int key, int value) = 0;
    virtual bool search(int key, int& value) = 0;
    virtual Node* remove(int key) = 0;
    virtual int get_size() const = 0;
    virtual int get_max_elements() const = 0;
    virtual Node* get_root() const { return root; }
    virtual std::vector<int> inorder() = 0;

    virtual Node* timed_insert(int key, int value, int& time) = 0;
    virtual bool timed_search(int key, int& value, int& time) = 0;
    virtual Node* timed_remove(int key, int& time) = 0;
    
    // New virtual methods for persistent key-value store functionality
    virtual std::vector<std::pair<int, int>> range_scan(int key1, int key2) = 0;
    // virtual bool open_database(const std::string& dbName) = 0;
    virtual bool close_database() = 0;
    virtual bool flush_to_sst() = 0;
    virtual void clear_memtable() = 0;
    virtual void setDatabaseName(const std::string& name) = 0;
    virtual void setDatabaseDirectory(const std::string& dir) = 0;
    virtual std::vector<std::pair<int, int>> range_scan_with_sst(int key1, int key2) = 0;
    virtual bool get(int key, int &value) = 0;
    // virtual bool create_directory(const std::string &path) = 0;
    virtual ~Memtable_ds() {}
};
std::unique_ptr<Memtable_ds> create_memtable(int maxElements); 
std::unique_ptr<Memtable_ds> create_memtable(std::vector<std::pair<int, int>> sst, int maxElements);


#endif // MEMTABLE_DS_HPP
