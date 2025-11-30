#include "FileOperations.hpp"
#include "AVL.hpp"
#include "BTree/BTreeSST.hpp"
#include <iostream>
#include <chrono>
#include <memory>
#include <algorithm>
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

    bool search_helper(Node* node, int key, int& value, bool verbose = true) {
        if (node == nullptr) {
            if (verbose) {
                std::cout << "Key " << key << " not found in memtable" << std::endl;
            }
            return false;
        }
        if (key == node->key) {
            value = node->value;
            if (verbose) {
                std::cout << "Key " << key << " found in memtable with value " << value << std::endl;
            }
            return true;
        }
        if (key < node->key) {
            return search_helper(node->left, key, value, verbose);
        }
        return search_helper(node->right, key, value, verbose);
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
        if (node->key >= key1 && node->key <= key2) {
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
        if (search_helper(root, key, dummy, false)) { // Silent duplicate check
            std::cout << "Error: Key " << key << " already exists in memtable." << endl;
            return nullptr;
        }
        root = insert_helper(root, key, value, currentSize);
        return root;
    }

    bool AVL::search(int key, int& value) {
        // First search in memtable (verbose)
        if (search_helper(root, key, value, true)) {
            return true;
        }
        // If not found in memtable, search in SST files
        return get_from_sst(key, value);
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
        bool result = search_helper(root, key, value, false); // Silent for timing tests
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
        
        if (key1 > key2) {
            std::cout << "Invalid range: key1 (" << key1 << ") must be less than or equal to key2 (" << key2 << ")" << std::endl;
            return result;
        }

        // Get results from memtable
        range_scan_helper(root, key1, key2, result);
        
        // Get results from SST files and merge
        std::vector<std::pair<int, int>> sst_results = range_scan_sst_files(key1, key2);
        
        // Merge results
        result.insert(result.end(), sst_results.begin(), sst_results.end());
        
        // Sort by key to maintain order
        std::sort(result.begin(), result.end());
        
        // Remove duplicates (keep first occurrence)
        auto last = std::unique(result.begin(), result.end(), 
            [](const std::pair<int,int>& a, const std::pair<int,int>& b) {
                return a.first == b.first;
            });
        result.erase(last, result.end());
        
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

    int AVL::get_next_file_number() const {
        return nextFileNumber;
    }

    // Method to search for a key in SST files
    bool AVL::get_from_sst(int key, int& value) {
        if (databaseDirectory.empty()) {
            return false;
        }
        
        // Get count of SST files and search through them
        int fileCount = FileOperations::count_sst_files(databaseDirectory);
        
        // Create B-Tree SST instance for querying
        BTreeSST btree;
        
        // Search through numbered SST files (1.txt, 2.txt, etc.)
        for (int i = 1; i <= fileCount; i++) {
            std::string filename = databaseDirectory + "/" + std::to_string(i) + ".txt";
            if (FileOperations::file_exists(filename)) {
                // Use B-Tree get function instead of reading entire file
                if (btree.get(key, value, filename)) {
                    std::cout << "Key " << key << " found in SST file " << filename << " with value " << value << std::endl;
                    return true;
                }
            }
        }
        
        std::cout << "Key " << key << " not found in any SST files" << std::endl;
        return false;
    }

    // Method to perform range scan on SST files
    std::vector<std::pair<int, int>> AVL::range_scan_sst_files(int key1, int key2) {
        std::vector<std::pair<int, int>> result;
        
        if (databaseDirectory.empty()) {
            return result;
        }
        
        // Get count of SST files and search through them
        int fileCount = FileOperations::count_sst_files(databaseDirectory);
        
        // Create B-Tree SST instance for querying
        BTreeSST btree;
        
        // Search through numbered SST files (1.txt, 2.txt, etc.)
        for (int i = 1; i <= fileCount; i++) {
            std::string filename = databaseDirectory + "/" + std::to_string(i) + ".txt";
            if (FileOperations::file_exists(filename)) {
                // Use B-Tree scan function instead of reading entire file
                std::vector<std::pair<int, int>> fileResults = btree.scan(key1, key2, filename);
                result.insert(result.end(), fileResults.begin(), fileResults.end());
            }
        }
        
        return result;
    }

    // Binary search implementation for SST files
    bool AVL::binary_search_sst(const std::vector<std::pair<int, int>>& pairs, int key, int& value) {
        int left = 0, right = (int)pairs.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (pairs[mid].first == key) {
                value = pairs[mid].second;
                return true;
            }
            if (pairs[mid].first < key) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        return false;
    }
