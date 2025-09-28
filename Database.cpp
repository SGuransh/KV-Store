#include "AVL.cpp"
#include <iostream>
#include <string>
#include <vector>
#include <utility>

class Database {
private:
    AVL *engine;   // underlying storage engine (AVL + SST)
    bool isOpen;

public:
    Database(int memtableCapacity = 1000) {
        engine = new AVL(memtableCapacity);
        isOpen = false;
    }

    ~Database() {
        if (isOpen) {
            closeDatabase();
        }
        delete engine;
    }

    bool openDatabase(const std::string &name) {
        if (isOpen) {
            std::cout << "Database already open!" << std::endl;
            return false;
        }
        if (engine->open_database(name)) {
            isOpen = true;
            return true;
        }
        return false;
    }

    bool closeDatabase() {
        if (!isOpen) {
            std::cout << "No database is currently open" << std::endl;
            return false;
        }
        bool result = engine->close_database();
        isOpen = false;
        return result;
    }

    bool put(int key, int value) {
        if (!isOpen) {
            std::cout << "Error: Database not open" << std::endl;
            return false;
        }
        return engine->insert(key, value) != nullptr;
    }

    bool get(int key, int &value) {
        if (!isOpen) {
            std::cout << "Error: Database not open" << std::endl;
            return false;
        }
        return engine->get(key, value);
    }

    std::vector<std::pair<int,int>> scan(int key1, int key2) {
        if (!isOpen) {
            std::cout << "Error: Database not open" << std::endl;
            return {};
        }
        return engine->range_scan_with_sst(key1, key2);
    }
};
