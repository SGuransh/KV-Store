#include <string>
#ifndef MEMTABLE_DS_HPP
#define MEMTABLE_DS_HPP

class Memtable_ds {
public:
    virtual void insert(int key, const std::string& value) = 0;
    virtual bool search(int key, std::string& value) = 0;
    virtual void remove(int key) = 0;
    virtual ~Memtable_ds() {}
};

#endif // MEMTABLE_DS_HPP