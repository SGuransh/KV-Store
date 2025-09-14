#include "AVL.cpp"
#include <iostream>
using namespace std;

int main() {
    AVL memtable;

    memtable.insert(1, "Value1");
    memtable.insert(2, "Value2");
    memtable.insert(3, "Value3");

    string value;
    if (memtable.search(2, value)) {
        cout << "Key 2 found with value: " << value << endl;
    } else {
        cout << "Key 2 not found." << endl;
    }

    memtable.remove(2);

    if (memtable.search(2, value)) {
        cout << "Key 2 found with value: " << value << endl;
    } else {
        cout << "Key 2 not found." << endl;
    }

    return 0;
}