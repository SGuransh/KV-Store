#include "Memtable_ds.hpp"
#include "FileOperations.hpp"
#include <iostream>
#include <chrono>
using namespace std;

class AVL : public Memtable_ds {
private:
    int currentSize = 0;
    int nextFileNumber = 1;
    std::string databaseDirectory;

    // Core AVL tree operations
    int height(Node* N) {
        if (N == nullptr) return 0;
        return N->height;
    }

    Node* newNode(const int key, const int value) {
        Node* node = new Node();
        node->key = key;
        node->value = value;
        node->left = nullptr;
        node->right = nullptr;
        node->height = 1;
        return node;
    }

    int BFactor(Node* N) {
        if (N == nullptr) return 0;
        return height(N->right) - height(N->left);
    }

    Node* rightRotation(Node* n) {
       Node* x = n->left;
       Node* moving = x->right;
       x->right = n;
       n->left = moving;
       n->height = 1 + max(height(n->left), height(n->right));
       x->height = 1 + max(height(x->left), height(x->right));
       return x;
    }

    Node* leftRotation(Node* n) {
       Node* x = n->right;
       Node* moving = x->left;
       x->left = n;
       n->right = moving;
       n->height = 1 + max(height(n->left), height(n->right));
       x->height = 1 + max(height(x->left), height(x->right));
       return x;
    }

    Node* insert(Node* node, int key, int value) {
        if (node == nullptr) {
            currentSize++;
            return newNode(key, value);
        }

        if (key > node->key) {
            node->right = insert(node->right, key, value);
        } else if (key < node->key) {
            node->left = insert(node->left, key, value);
        } else {
            return nullptr; // Duplicate key
        }

        node->height = 1 + max(height(node->left), height(node->right));
        int balance = BFactor(node);

        // Perform rotations if needed
        if (balance == 2 && BFactor(node->right) >= 0) {
            return leftRotation(node);
        } else if (balance == 2 && BFactor(node->right) < 0) {
            node->right = rightRotation(node->right);
            return leftRotation(node);
        } else if (balance == -2 && BFactor(node->left) <= 0) {
            return rightRotation(node);
        } else if (balance == -2 && BFactor(node->left) > 0) {
            node->left = leftRotation(node->left);
            return rightRotation(node);
        }
        return node;
    }

    bool search(Node* node, int key, int& value) {
        if (node == nullptr) return false;
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

    void range_scan_helper(Node* node, int key1, int key2, std::vector<std::pair<int, int>>& result) {
        if (node == nullptr) return;

        if (node->key > key1) {
            range_scan_helper(node->left, key1, key2, result);
        }
        if (node->key > key1 && node->key < key2) {
            result.push_back(std::make_pair(node->key, node->value));
        }
        if (node->key < key2) {
            range_scan_helper(node->right, key1, key2, result);
        }
    }

    void collect_all_pairs(Node* node, std::vector<std::pair<int, int>>& pairs) {
        if (node == nullptr) return;
        collect_all_pairs(node->left, pairs);
        pairs.push_back(std::make_pair(node->key, node->value));
        collect_all_pairs(node->right, pairs);
    }

    void clear_memtable() {
        if (root != nullptr) {
            delete_tree(root);
            root = nullptr;
        }
        currentSize = 0;
    }

    void delete_tree(Node* node) {
        if (node != nullptr) {
            delete_tree(node->left);
            delete_tree(node->right);
            delete node;
        }
    }

public:
    AVL(int maxElements) : Memtable_ds(maxElements) {
        root = nullptr; 
        currentSize = 0;
    }

    AVL(std::vector<std::pair<int, int>> sst, int maxElements) : Memtable_ds(sst, maxElements) {
        root = nullptr;
        currentSize = 0;
        for (const auto& pair : sst) {
            insert(pair.first, pair.second); 
        }
    }

    // Interface implementations
    Node* insert(int key, int value) override {
        if (currentSize >= maxElements) {
            cout << "Memtable is at capacity (" << currentSize << "/" << maxElements << "), attempting to flush to SST" << endl;
            
            if (!flush_to_sst(nextFileNumber, true)) {
                cout << "Error: Failed to flush memtable to SST file." << endl;
                return nullptr;
            }
            
            cout << "Successfully flushed memtable to SST. Proceeding with insertion." << endl;
            nextFileNumber++;
        }
        
        cout << "Inserting key: " << key << " with value: " << value << endl;
        
        int dummy;
        if (search(root, key, dummy)) {
            cout << "Error: Key " << key << " already exists in memtable." << endl;
            return nullptr;
        }
        
        root = insert(root, key, value);
        return root;
    }

    bool search(int key, int& value) override {
        return search(root, key, value);
    }

    Node* remove(int key) override {
        cout << "Remove operation not implemented" << endl;
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
        Node* result = insert(key, value);
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return result;
    }

    bool timed_search(int key, int& value, int& time) override {
        auto start = std::chrono::high_resolution_clock::now();
        bool result = search(key, value);
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return result;
    }

    Node* timed_remove(int key, int& time) override {
        auto start = std::chrono::high_resolution_clock::now();
        Node* result = remove(key);
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return result;
    }

    std::vector<std::pair<int, int>> range_scan(int key1, int key2) override {
        std::vector<std::pair<int, int>> result;
        
        if (key1 >= key2) {
            cout << "Invalid range: key1 (" << key1 << ") must be less than key2 (" << key2 << ")" << endl;
            return result;
        }

        range_scan_helper(root, key1, key2, result);
        cout << "Range scan from " << key1 << " to " << key2 << " found " << result.size() << " elements" << endl;
        return result;
    }

    bool flush_to_sst(int fileNumber, bool isComplete = true) override {
        cout << "Flushing memtable to SST file" << endl;

        if (currentSize == 0 || root == nullptr) {
            cout << "Memtable is empty, nothing to flush" << endl;
            return true;
        }

        std::vector<std::pair<int, int>> pairs;
        collect_all_pairs(root, pairs);

        if (pairs.empty()) {
            cout << "No pairs collected from memtable" << endl;
            return true;
        }

        // Create filename based on completion status
        std::string filename;
        if (isComplete) {
            filename = databaseDirectory + "/" + std::to_string(fileNumber) + ".txt";
        } else {
            filename = databaseDirectory + "/incomplete.txt";
        }

        bool writeSuccess = FileOperations::write_sst_file(pairs, filename, true);
        
        if (!writeSuccess) {
            cout << "Failed to write SST file, memtable not cleared" << endl;
            return false;
        }

        clear_memtable();
        cout << "Successfully flushed " << pairs.size() << " entries to SST file" << endl;
        return true;
    }

    bool load_from_sst(const std::vector<std::pair<int, int>>& data) override {
        cout << "Loading " << data.size() << " entries into memtable" << endl;
        
        clear_memtable();
        
        for (const auto& pair : data) {
            root = insert(root, pair.first, pair.second);
            if (root == nullptr) {
                cout << "Failed to insert key " << pair.first << " during SST load" << endl;
                return false;
            }
        }
        
        cout << "Successfully loaded " << currentSize << " entries from SST data" << endl;
        return true;
    }

    void set_next_file_number(int nextFileNum) override {
        nextFileNumber = nextFileNum;
        cout << "Set next file number to: " << nextFileNumber << endl;
    }

    void set_database_directory(const std::string& dbDir) override {
        databaseDirectory = dbDir;
    }
};