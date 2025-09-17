#include "AVL.cpp"
#include <iostream>

int main() {
    int maxElements = 6;
    Memtable_ds* memtable = new AVL(maxElements);
    memtable->insert(16, 100);
    memtable->insert(7, 200);
    memtable->insert(53, 300);
    memtable->insert(30, 400);
    memtable->insert(55, 500);
    memtable->insert(29, 600); 

    // Checking the table
    if (memtable->get_size() != 6) {
        std::cout << "Size check failed!" << std::endl;
    } else {
        std::cout << "Size check passed!" << std::endl;

    } 

    if (memtable->get_root() == nullptr) {
        std::cout << "Root check failed!" << std::endl;
    } else if (memtable->get_root()->key != 30) {
        std::cout << "Root key check failed! Expected 30, got " << memtable->get_root()->key << std::endl;
    } else {
        std::cout << "Root check passed!" << std::endl;
    }

    if (memtable->get_max_elements() != 6) {
        std::cout << "Max elements check failed!" << std::endl;
    } else {
        std::cout << "Max elements check passed!" << std::endl;
    }

    printf("Inorder traversal of the AVL tree: ");
    std::vector<int> inorder_result = memtable->inorder();
    for (int key : inorder_result) {
        std::cout << key << " ";
    }
    
    return 0;
}