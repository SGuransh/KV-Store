#include <string>
#ifndef MEMTABLE_DS_HPP
#define MEMTABLE_DS_HPP

#include <vector>

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
    virtual ~Memtable_ds() {}
};

#endif // MEMTABLE_DS_HPP