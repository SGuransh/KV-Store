#include "BTree/BTreeSST.hpp"
#include <iostream>

int main() {
    BTreeSST reader;
    auto data1 = reader.toSortedArray("test_merge_overlap/sst_1.txt");
    auto data2 = reader.toSortedArray("test_merge_overlap/sst_2.txt");
    
    std::cout << "SST1 has " << data1.size() << " pairs:" << std::endl;
    for (const auto& p : data1) {
        std::cout << "  " << p.first << ":" << p.second << std::endl;
    }
    
    std::cout << "\nSST2 has " << data2.size() << " pairs:" << std::endl;
    for (const auto& p : data2) {
        std::cout << "  " << p.first << ":" << p.second << std::endl;
    }
    
    return 0;
}
