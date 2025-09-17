#include "Memtable_ds.hpp"
#include <iostream>
#include <unordered_map>
#include <chrono>
using namespace std;

class AVL : public Memtable_ds {
private:

    int currentSize = 0;

    int height(Node* N) {
        /*
            Simple Heigh calculating helper function
        */
        if (N == nullptr)
            return 0;
        return N->height;
    }

    Node* newNode(const int key, const int value) {
        /*
            Helper function to make new Nodes
        */
        Node* node = new Node();
        node->key = key;
        node->value = value;
        node->left = nullptr;
        node->right = nullptr;
        node->height = 1;
        return node;
    }

    int BFactor(Node* N) {
        /*
            Returns the Balance Factor of node N, which is height of right subtree - height of left subtree.
        */
        if (N == nullptr)
            return 0;
        return height(N->right) - height(N->left);
    }

    Node* rightRotatation(Node* n) {
        /*
            This function performs a right rotation on the given node y and returns the new root of the subtree.
        */
       Node* x = n->left;
       Node* moving = x->right;

       x->right = n;
       n->left = moving;

       n->height = 1 + max(height(n->left), height(n->right));
       x->height = 1 + max(height(x->left), height(x->right));

       return x;
    }

    Node* leftRotatation(Node* n) {
        /*
            This function performs a left rotation on the given node x and returns the new root of the subtree.
        */
       Node* x = n->right;
       Node* moving = x->left;

       x->left = n;
       n->right = moving;

       n->height = 1 + max(height(n->left), height(n->right));
       x->height = 1 + max(height(x->left), height(x->right));

       return x;
    }

    Node* insert(Node* node, int key, int value){
        /*
            This is the insertion function. The Algo is:
            1. Perform the normal BST insertion.
            2. Update the height of this ancestor nodes.
            3. Get the balance factor of this ancestor nodes to check whether this node became unbalanced.

            Returns the head of the modified tree.
        */
        if (currentSize >= maxElements) {
            cout << "Memtable is full. Cannot insert new elements." << endl;
            return nullptr;
        }

        if (node == nullptr){
            currentSize++;
            return newNode(key, value);
        }

        if (key > node->key){
            node->right = insert(node->right, key, value);
        }
        else if (key < node->key){
            node->left = insert(node->left, key, value);
        }
        else{
            return nullptr;
        }

        // Updating the height of the node
        node->height = 1 + max(height(node->left), height(node->right));

        int balance = BFactor(node);

        // Balance factor being 2 or -2 means that we need rotations    
        if (balance == 2 && BFactor(node->right) >= 0){
            return leftRotatation(node);
        }
        else if (balance == 2 && BFactor(node->right) < 0){
            node->right = rightRotatation(node->right);
            return leftRotatation(node);
        }
        else if (balance == -2 && BFactor(node->left) <= 0){
            return rightRotatation(node);
        }
        else if (balance == -2 && BFactor(node->left) > 0){
            node->left = leftRotatation(node->left);
            return rightRotatation(node);
        }
        return node;
    }

    bool search(Node* node, int key, int& value) {
        /*
            This is the search function. The Algo is:
            1. Start from the root and compare the key with the key of the current node.
            2. If the keys are equal, return the value.
            3. If the key is smaller than the current node's key, go to the left subtree.
            4. If the key is larger than the current node's key, go to the right subtree.
            5. If we reach a null node, the key is not present in the tree.

            Returns true if found, false otherwise.
        */
        if (node == nullptr) {
            return false;
        }
        if (key == node->key) {
            value = node->value;
            return true;
        }
        if (key < node->key) {
            return search(node->left, key, value);
        }
        return search(node->right, key, value);
    }

    void inorder_helper(Node* node, std::vector<int>& result) {
        if (node != nullptr) {
            inorder_helper(node->left, result);
            result.push_back(node->value);
            inorder_helper(node->right, result);
        }
    }

public:
    AVL(int maxElements) : Memtable_ds(maxElements) {root = nullptr; currentSize = 0;}

    Node* insert(int key, int value) override {
        if (currentSize >= maxElements) {
            cout << "Memtable is full. Cannot insert new elements." << endl;
            return nullptr;
        }
        cout << "Inserting key: " << key << " with value: " << value << endl;
        int dummy;
        if (search(root, key, dummy)) {
            cout << "Key " << key << " already exists. Insertion aborted." << endl;
            return nullptr;
        }
        root = insert(root, key, value);
        return root;
    }

    bool search(int key, int& value) override {
        return search(root, key, value);
    }

    Node* remove(int key) override {
        cout << "Removing key: " << key << endl;
        return nullptr;
    }

    int get_size() const override {
        return currentSize;
    }

    int get_max_elements() const override {
        return maxElements;
    }

    std::vector<int> inorder() override {
        std::vector<int> result;
        inorder_helper(root, result);
        return result;
    }

    Node* timed_insert(int key, int value, int& time) override {
        auto start = std::chrono::high_resolution_clock::now();
        cout << "Timed insert for key: " << key << " with value: " << value << endl;
        Node* result = insert(key, value);
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if (result == nullptr) {
            cout << "Insertion failed. Memtable might be full or key already exists." << endl;
            return nullptr;
        }
        return result;
    }

    bool timed_search(int key, int& value, int& time) override {
        auto start = std::chrono::high_resolution_clock::now();
        cout << "Timed search for key: " << key << endl;
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return false;
    }

    Node* timed_remove(int key, int& time) override {
        auto start = std::chrono::high_resolution_clock::now();
        cout << "Timed remove for key: " << key << endl;
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return nullptr;
    }
};