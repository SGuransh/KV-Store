#include "FileOperations.hpp"
#include "AVL.hpp"
#include <iostream>
#include <chrono>
#include <memory>
using namespace std;

    // int currentSize = 0;
    // int nextFileNumber = 1;
    // std::string databaseDirectory;

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

    Node* insert_helper(Node* node, int key, int value, int& currentSize) {
        if (node == nullptr) {
            currentSize += 1;
            return newNode(key, value);
        }

        if (key > node->key) {
            node->right = insert_helper(node->right, key, value, currentSize);
        } else if (key < node->key) {
            node->left = insert_helper(node->left, key, value, currentSize);
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

    bool search_helper(Node* node, int key, int& value) {
        if (node == nullptr) {
            std::cout << "Key " << key << " not found" << std::endl;
            return false;
        }
        if (key == node->key) {
            value = node->value;
            return true;
        }
        if (key < node->key) {
            return search_helper(node->left, key, value);
        }
        return search_helper(node->right, key, value);
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

    void delete_tree(Node* node) {
        if (node != nullptr) {
            delete_tree(node->left);
            delete_tree(node->right);
            delete node;
        }
    }

    void clear_memtable(Node* root, int& currentSize) {
        if (root != nullptr) {
            delete_tree(root);
            root = nullptr;
        }
        currentSize = 0;
    }

// -----------------------------------------------------------------------------------------------------------------------------------------------
// _______________________________________________________________________________________________________________________________________________

    AVL::AVL(int maxElements) : Memtable_ds(maxElements) {
        root = nullptr; 
        currentSize = 0;
    }

    AVL::AVL(std::vector<std::pair<int, int>> sst, int maxElements) : Memtable_ds(sst, maxElements) {
        root = nullptr;
        currentSize = 0;
        for (const auto& pair : sst) {
            insert(pair.first, pair.second); 
        }
    }

    // Interface implementations
    Node* AVL::insert(int key, int value) {
        if (currentSize >= maxElements) {
            std::cout << "Memtable is at capacity (" << currentSize << "/" << maxElements << "), attempting to flush to SST" << endl;
            if (!flush_to_sst(nextFileNumber, true)) {
                std::cout << "Error: Failed to flush memtable to SST file." << endl;
                return nullptr;
            }
            std::cout << "Successfully flushed memtable to SST. Proceeding with insertion." << endl;
            nextFileNumber++;

            root = insert_helper(root, key, value, currentSize);
            return root;
        }
        std::cout << "Inserting key: " << key << " with value: " << value << endl;
        int dummy;
        if (search_helper(root, key, dummy)) {
            std::cout << "Error: Key " << key << " already exists in memtable." << endl;
            return nullptr;
        }
        std::cout << "Search complete" << std::endl;
        root = insert_helper(root, key, value, currentSize);
        return root;
    }

    bool AVL::search(int key, int& value) {
        return search_helper(root, key, value);
    }

    Node* AVL::remove(int key) {
        std::cout << "Remove operation not implemented" << std::endl;
        return nullptr;
    }

    int AVL::get_size() const {
        return currentSize;
    }

    int AVL::get_max_elements() const {
        return maxElements;
    }

    std::vector<int> AVL::inorder() {
        std::vector<int> result;
        inorder_helper(root, result);
        return result;
    }

    Node* AVL::timed_insert(int key, int value, int& time) {
        auto start = std::chrono::high_resolution_clock::now();
        Node* result = insert(key, value);
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return result;
    }

    bool AVL::timed_search(int key, int& value, int& time) {
        auto start = std::chrono::high_resolution_clock::now();
        bool result = search_helper(root, key, value);
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return result;
    }

    Node* AVL::timed_remove(int key, int& time) {
        auto start = std::chrono::high_resolution_clock::now();
        Node* result = remove(key);
        auto end = std::chrono::high_resolution_clock::now();
        time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        return result;
    }

    std::vector<std::pair<int, int>> AVL::range_scan(int key1, int key2) {
        std::vector<std::pair<int, int>> result;
        
        if (key1 >= key2) {
            std::cout << "Invalid range: key1 (" << key1 << ") must be less than key2 (" << key2 << ")" << std::endl;
            return result;
        }

        range_scan_helper(root, key1, key2, result);
        std::cout << "Range scan from " << key1 << " to " << key2 << " found " << result.size() << " elements" << std::endl;
        return result;
    }

    bool AVL::flush_to_sst(int fileNumber, bool isComplete) {
        std::cout << "Flushing memtable to SST file" << std::endl;

        if (currentSize == 0 || root == nullptr) {
            std::cout << "Memtable is empty, nothing to flush" << std::endl;
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

        clear_memtable(root, currentSize);
        root = nullptr;
        currentSize = 0;
        return true;
    }

    bool AVL::load_from_sst(const std::vector<std::pair<int, int>>& data) {
        std::cout << "Loading " << data.size() << " entries into memtable" << std::endl;

        clear_memtable(root, currentSize);
        root = nullptr;
        currentSize = 0;

        // Node* tree = new AVL(data, maxElements).get_root();
        // currentSize = data.size();
        
        for (const auto& pair : data) {
            root = insert_helper(root, pair.first, pair.second, currentSize);
            if (root == nullptr) {
                std::cout << "Failed to insert key " << pair.first << " during SST load" << std::endl;
                return false;
            }
        }

        std::cout << "Successfully loaded " << currentSize << " entries from SST data" << std::endl;
        return true;
    }

    void AVL::set_next_file_number(int nextFileNum) {
        nextFileNumber = nextFileNum;
        std::cout << "Set next file number to: " << nextFileNumber << std::endl;
    }

    void AVL::set_database_directory(const std::string& dbDir) {
        databaseDirectory = dbDir;
    }
