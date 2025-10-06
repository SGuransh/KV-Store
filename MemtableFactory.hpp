#pragma once
#include "Memtable_ds.hpp"
#include "AVL.hpp"
// #include "BTree.hpp"
#include <memory>
#include <vector>
#include <string>

enum class MemtableType { AVL, BTREE };

inline std::unique_ptr<Memtable_ds> create_memtable(MemtableType type, int maxElements) {
    switch(type) {
        case MemtableType::AVL: return std::make_unique<AVL>(maxElements);
        case MemtableType::BTREE: return nullptr;
    }
    return nullptr;
}

inline std::unique_ptr<Memtable_ds> create_memtable(MemtableType type, const std::vector<std::pair<int,int>>& sst, int maxElements) {
    switch(type) {
        case MemtableType::AVL: return std::make_unique<AVL>(sst, maxElements);
        case MemtableType::BTREE: return nullptr;
    }
    return nullptr;
}