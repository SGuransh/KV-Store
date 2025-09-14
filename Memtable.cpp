#include "Memtable_ds.hpp"
#include <iostream>
#include <memory> // For std::unique_ptr
#include "AVL.cpp" // Include the AVL class definition
using namespace std;

int main() {
    int maxElements = 3; // Define the maximum number of elements
    unique_ptr<Memtable_ds> memtable = make_unique<AVL>(maxElements);

    cout << "Inserting elements up to the limit:\n";
    memtable->insert(1, "Value1");
    memtable->insert(2, "Value2");
    memtable->insert(3, "Value3");

    cout << "Attempting to insert beyond the limit:\n";
    memtable->insert(4, "Value4"); // Should fail

    cout << "Searching for an existing key (2):\n";
    string value;
    if (memtable->search(2, value)) {
        cout << "Key 2 found with value: " << value << endl;
    } else {
        cout << "Key 2 not found." << endl;
    }

    cout << "Searching for a non-existing key (5):\n";
    if (memtable->search(5, value)) {
        cout << "Key 5 found with value: " << value << endl;
    } else {
        cout << "Key 5 not found." << endl;
    }

    cout << "Deleting an existing key (2):\n";
    memtable->remove(2);

    cout << "Searching for the deleted key (2):\n";
    if (memtable->search(2, value)) {
        cout << "Key 2 found with value: " << value << endl;
    } else {
        cout << "Key 2 not found." << endl;
    }

    cout << "Attempting to delete a non-existing key (5):\n";
    memtable->remove(5); // Should handle gracefully

    return 0;
}