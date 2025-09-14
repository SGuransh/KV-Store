#include "Memtable_ds.hpp"
#include <iostream>
#include <unordered_map>
using namespace std;

class AVL : public Memtable_ds {
private:
    struct Node {
        int key;
        string value;
        Node* left;
        Node* right;
        int height;
    };

    Node* root;
    int currentSize; // Track the current number of elements

    int height(Node* N) {
        if (N == nullptr)
            return 0;
        return N->height;
    }

    Node* newNode(int key, const string& value) {
        Node* node = new Node();
        node->key = key;
        node->value = value;
        node->left = nullptr;
        node->right = nullptr;
        node->height = 1;
        return node;
    }

    Node* rightRotate(Node* y) {
        Node* x = y->left;
        Node* T2 = x->right;

        x->right = y;
        y->left = T2;

        y->height = max(height(y->left), height(y->right)) + 1;
        x->height = max(height(x->left), height(x->right)) + 1;

        return x;
    }

    Node* leftRotate(Node* x) {
        Node* y = x->right;
        Node* T2 = y->left;

        y->left = x;
        x->right = T2;

        x->height = max(height(x->left), height(x->right)) + 1;
        y->height = max(height(y->left), height(y->right)) + 1;

        return y;
    }

    int getBalance(Node* N) {
        if (N == nullptr)
            return 0;
        return height(N->left) - height(N->right);
    }

    Node* insert(Node* node, int key, const string& value) {
        if (node == nullptr)
            return newNode(key, value);

        if (key < node->key)
            node->left = insert(node->left, key, value);
        else if (key > node->key)
            node->right = insert(node->right, key, value);
        else {
            node->value = value; // Update value if key already exists
            return node;
        }

        node->height = 1 + max(height(node->left), height(node->right));

        int balance = getBalance(node);

        if (balance > 1 && key < node->left->key)
            return rightRotate(node);

        if (balance < -1 && key > node->right->key)
            return leftRotate(node);

        if (balance > 1 && key > node->left->key) {
            node->left = leftRotate(node->left);
            return rightRotate(node);
        }

        if (balance < -1 && key < node->right->key) {
            node->right = rightRotate(node->right);
            return leftRotate(node);
        }

        return node;
    }

    Node* remove(Node* node, int key) {
        if (node == nullptr)
            return node;

        if (key < node->key)
            node->left = remove(node->left, key);
        else if (key > node->key)
            node->right = remove(node->right, key);
        else {
            if ((node->left == nullptr) || (node->right == nullptr)) {
                Node* temp = node->left ? node->left : node->right;

                if (temp == nullptr) {
                    temp = node;
                    node = nullptr;
                } else
                    *node = *temp;

                delete temp;
            } else {
                Node* temp = minValueNode(node->right);
                node->key = temp->key;
                node->value = temp->value;
                node->right = remove(node->right, temp->key);
            }
        }

        if (node == nullptr)
            return node;

        node->height = 1 + max(height(node->left), height(node->right));

        int balance = getBalance(node);

        if (balance > 1 && getBalance(node->left) >= 0)
            return rightRotate(node);

        if (balance > 1 && getBalance(node->left) < 0) {
            node->left = leftRotate(node->left);
            return rightRotate(node);
        }

        if (balance < -1 && getBalance(node->right) <= 0)
            return leftRotate(node);

        if (balance < -1 && getBalance(node->right) > 0) {
            node->right = rightRotate(node->right);
            return leftRotate(node);
        }

        return node;
    }

    Node* minValueNode(Node* node) {
        Node* current = node;
        while (current->left != nullptr)
            current = current->left;
        return current;
    }

    bool search(Node* node, int key, string& value) {
        if (node == nullptr)
            return false;
        if (key == node->key) {
            value = node->value;
            return true;
        }
        if (key < node->key)
            return search(node->left, key, value);
        return search(node->right, key, value);
    }

public:
    AVL(int maxElements) : Memtable_ds(maxElements), root(nullptr), currentSize(0) {}

    void insert(int key, const string& value) override {
        if (currentSize >= maxElements) {
            cout << "Memtable is full. Cannot insert new elements." << endl;
            return;
        }
        root = insert(root, key, value);
        currentSize++;
    }

    bool search(int key, string& value) override {
        return search(root, key, value);
    }

    void remove(int key) override {
        root = remove(root, key);
        currentSize--;
    }
};