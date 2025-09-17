#include "Memtable_ds.hpp"
#include <iostream>
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
using namespace std;

class AVL : public Memtable_ds {
private:

    int currentSize = 0;
    std::string databaseName;
    std::string databaseDirectory;

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

    void range_scan_helper(Node* node, int key1, int key2, std::vector<std::pair<int, int>>& result) {
        /*
            Recursive helper function that traverses AVL tree with boundary checking.
            Collects key-value pairs where key1 < key < key2 during in-order traversal.
        */
        if (node == nullptr) {
            return;
        }

        // If current node's key is greater than key1, traverse left subtree
        if (node->key > key1) {
            range_scan_helper(node->left, key1, key2, result);
        }

        // If current node's key is in range (key1 < key < key2), add to result
        if (node->key > key1 && node->key < key2) {
            result.push_back(std::make_pair(node->key, node->value));
        }

        // If current node's key is less than key2, traverse right subtree
        if (node->key < key2) {
            range_scan_helper(node->right, key1, key2, result);
        }
    }

    void collect_all_pairs(Node* node, std::vector<std::pair<int, int>>& pairs) {
        /*
            Recursive function to collect all key-value pairs from AVL tree in sorted order.
            Uses in-order traversal to maintain ascending key order.
        */
        if (node == nullptr) {
            return;
        }

        // In-order traversal: left, current, right
        collect_all_pairs(node->left, pairs);
        pairs.push_back(std::make_pair(node->key, node->value));
        collect_all_pairs(node->right, pairs);
    }

    int count_sst_files() {
        /*
            Function to count existing SST files in database directory.
            Handles case where directory doesn't exist yet.
            Returns count for determining next file number.
        */
        if (databaseDirectory.empty()) {
            return 0;
        }

        // Check if directory exists using stat
        struct stat st;
        if (stat(databaseDirectory.c_str(), &st) != 0) {
            return 0;
        }

        int count = 0;
        DIR* dir = opendir(databaseDirectory.c_str());
        if (dir == nullptr) {
            cout << "Error opening directory: " << databaseDirectory << endl;
            return 0;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            // Check if file matches SST pattern (number.txt)
            if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".txt") {
                std::string numberPart = filename.substr(0, filename.length() - 4);
                // Check if the filename before .txt is a number
                bool isNumber = true;
                for (char c : numberPart) {
                    if (!std::isdigit(c)) {
                        isNumber = false;
                        break;
                    }
                }
                if (isNumber && !numberPart.empty()) {
                    count++;
                }
            }
        }
        closedir(dir);

        return count;
    }

    bool write_sst_file(const std::vector<std::pair<int, int>>& data, int fileNumber) {
        /*
            Function to create SST file with given data and file number.
            Uses simple text format: "key value" per line.
            Ensures atomic write operation (write to temp file, then rename).
        */
        if (databaseDirectory.empty()) {
            cout << "Database directory not set" << endl;
            return false;
        }

        // Create the SST filename
        std::string filename = std::to_string(fileNumber) + ".txt";
        std::string filepath = databaseDirectory + "/" + filename;
        std::string tempFilepath = filepath + ".tmp";

        try {
            // Write to temporary file first for atomic operation
            std::ofstream tempFile(tempFilepath);
            if (!tempFile.is_open()) {
                cout << "Failed to create temporary SST file: " << tempFilepath << endl;
                return false;
            }

            // Write data in simple text format: "key value" per line
            for (const auto& pair : data) {
                tempFile << pair.first << " " << pair.second << "\n";
            }

            tempFile.close();

            // Check if write was successful
            if (tempFile.fail()) {
                cout << "Failed to write data to temporary SST file" << endl;
                std::remove(tempFilepath.c_str()); // Clean up temp file
                return false;
            }

            // Atomically rename temp file to final file
            if (std::rename(tempFilepath.c_str(), filepath.c_str()) != 0) {
                cout << "Failed to rename temporary file to final file" << endl;
                std::remove(tempFilepath.c_str()); // Clean up temp file
                return false;
            }
            
            cout << "Successfully wrote SST file: " << filepath << " with " << data.size() << " entries" << endl;
            return true;

        } catch (const std::exception& ex) {
            cout << "Error writing SST file: " << ex.what() << endl;
            // Clean up temp file if it exists
            struct stat st;
            if (stat(tempFilepath.c_str(), &st) == 0) {
                std::remove(tempFilepath.c_str());
            }
            return false;
        }
    }

    void clear_memtable() {
        /*
            Method to reset memtable state after flush.
            Resets currentSize to 0 and root to nullptr.
            Properly deallocates existing tree nodes to prevent memory leaks.
        */
        if (root != nullptr) {
            delete_tree(root);
            root = nullptr;
        }
        currentSize = 0;
    }

    void delete_tree(Node* node) {
        /*
            Helper method to recursively delete all nodes in the tree.
        */
        if (node != nullptr) {
            delete_tree(node->left);
            delete_tree(node->right);
            delete node;
        }
    }

    bool create_directory(const std::string& path) {
        /*
            Function to create database directory if it doesn't exist.
            Handles directory creation errors gracefully.
            Uses POSIX directory creation approach.
        */
        try {
            // Check if directory already exists
            struct stat st;
            if (stat(path.c_str(), &st) == 0) {
                if (S_ISDIR(st.st_mode)) {
                    cout << "Directory already exists: " << path << endl;
                    return true;
                } else {
                    cout << "Path exists but is not a directory: " << path << endl;
                    return false;
                }
            }

            // Create directory with permissions 0755
            if (mkdir(path.c_str(), 0755) == 0) {
                cout << "Successfully created directory: " << path << endl;
                return true;
            } else {
                cout << "Failed to create directory: " << path << endl;
                return false;
            }

        } catch (const std::exception& ex) {
            cout << "Unexpected error creating directory " << path << ": " << ex.what() << endl;
            return false;
        }
    }

public:
    AVL(int maxElements) : Memtable_ds(maxElements) {root = nullptr; currentSize = 0;}

    Node* insert(int key, int value) override {
        // Check if memtable is at capacity before insertion
        if (currentSize >= maxElements) {
            cout << "Memtable is at capacity (" << currentSize << "/" << maxElements << "), attempting to flush to SST" << endl;
            
            // If at capacity, call flush_to_sst to create SST file
            if (!flush_to_sst()) {
                cout << "Error: Failed to flush memtable to SST file. Memtable remains full and cannot accept new insertions." << endl;
                cout << "Possible causes: Database not opened, disk space issues, or file permission problems." << endl;
                return nullptr;
            }
            
            cout << "Successfully flushed memtable to SST. Proceeding with insertion in cleared memtable." << endl;
        }
        
        cout << "Inserting key: " << key << " with value: " << value << endl;
        
        // Maintain existing duplicate key prevention logic
        int dummy;
        if (search(root, key, dummy)) {
            cout << "Error: Key " << key << " already exists in memtable. Duplicate keys are not allowed." << endl;
            return nullptr;
        }
        
        // Perform the insertion
        root = insert(root, key, value);
        
        if (root == nullptr) {
            cout << "Error: Insertion failed for key " << key << ". This should not happen after capacity and duplicate checks." << endl;
            return nullptr;
        }
        
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

    std::vector<std::pair<int, int>> range_scan(int key1, int key2) override {
        /*
            Public interface for range scanning that calls range_scan_helper.
            Returns all key-value pairs from memtable where key1 < key < key2.
            Handles edge cases like invalid ranges (key1 >= key2).
        */
        std::vector<std::pair<int, int>> result;
        
        // Handle edge case: invalid range
        if (key1 >= key2) {
            cout << "Invalid range: key1 (" << key1 << ") must be less than key2 (" << key2 << ")" << endl;
            return result; // Return empty vector
        }

        // Perform range scan using helper function
        range_scan_helper(root, key1, key2, result);
        
        cout << "Range scan from " << key1 << " to " << key2 << " found " << result.size() << " elements" << endl;
        return result;
    }

    bool open_database(const std::string& dbName) override {
        /*
            Sets database name and constructs directory path.
            Creates directory if it doesn't exist using create_directory.
            Initializes empty memtable for new operations.
        */
        cout << "Opening database: " << dbName << endl;

        // Validate database name
        if (dbName.empty()) {
            cout << "Error: Database name cannot be empty" << endl;
            return false;
        }

        // Check for invalid characters in database name (basic validation)
        for (char c : dbName) {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                cout << "Error: Database name contains invalid characters: " << dbName << endl;
                return false;
            }
        }

        // Set database name and construct directory path
        databaseName = dbName;
        databaseDirectory = databaseName; // Simple approach: directory name = database name

        // Create directory if it doesn't exist
        if (!create_directory(databaseDirectory)) {
            cout << "Failed to create or access database directory: " << databaseDirectory << endl;
            databaseName.clear();
            databaseDirectory.clear();
            return false;
        }

        // Initialize empty memtable for new operations
        // Clear any existing data first
        clear_memtable();

        cout << "Successfully opened database: " << databaseName << " at directory: " << databaseDirectory << endl;
        return true;
    }

    bool close_database() override {
        /*
            Checks if memtable has data using get_size().
            If memtable not empty, calls flush_to_sst to persist data.
            Cleans up resources and resets state.
        */
        cout << "Closing database: " << databaseName << endl;

        // Check if database is open
        if (databaseName.empty()) {
            cout << "No database is currently open" << endl;
            return true; // Not an error, just nothing to close
        }

        bool success = true;

        // Check if memtable has data using get_size()
        if (get_size() > 0) {
            cout << "Memtable contains " << get_size() << " entries, flushing to SST before close" << endl;
            
            // If memtable not empty, call flush_to_sst to persist data
            if (!flush_to_sst()) {
                cout << "Error: Failed to flush memtable data during database close" << endl;
                success = false;
            } else {
                cout << "Successfully flushed memtable data to SST file" << endl;
            }
        } else {
            cout << "Memtable is empty, no data to flush" << endl;
        }

        // Clean up resources and reset state
        clear_memtable();
        databaseName.clear();
        databaseDirectory.clear();

        if (success) {
            cout << "Database closed successfully" << endl;
        } else {
            cout << "Database closed with errors (data may have been lost)" << endl;
        }

        return success;
    }

    bool flush_to_sst() override {
        /*
            Collects all pairs from current memtable using collect_all_pairs.
            Determines next file number using count_sst_files.
            Writes data to SST file using write_sst_file.
            Clears memtable after successful write.
        */
        cout << "Flushing memtable to SST file" << endl;

        // Check if memtable has any data
        if (currentSize == 0 || root == nullptr) {
            cout << "Memtable is empty, nothing to flush" << endl;
            return true; // Not an error, just nothing to do
        }

        // Collect all key-value pairs from memtable
        std::vector<std::pair<int, int>> pairs;
        collect_all_pairs(root, pairs);

        if (pairs.empty()) {
            cout << "No pairs collected from memtable" << endl;
            return true; // Not an error, just nothing to do
        }

        // Determine next file number
        int fileNumber = count_sst_files() + 1;

        // Write data to SST file
        bool writeSuccess = write_sst_file(pairs, fileNumber);
        
        if (!writeSuccess) {
            cout << "Failed to write SST file, memtable not cleared" << endl;
            return false;
        }

        // Clear memtable only after successful write
        clear_memtable();
        
        cout << "Successfully flushed " << pairs.size() << " entries to SST file " << fileNumber << ".txt" << endl;
        return true;
    }
};