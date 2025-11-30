#include "Database.hpp"
#include "DBConfig.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <iomanip>

void printHelp() {
    std::cout << "\n=== KV-Store Database Commands ===" << std::endl;
    std::cout << "  open <db_name>            - Open/create a database" << std::endl;
    std::cout << "  close                     - Close current database" << std::endl;
    std::cout << "  insert/i <key> <value>    - Insert a key-value pair" << std::endl;
    std::cout << "  seq <start> <end> <step>  - Insert sequential K-V pairs that are identical" << std::endl;
    std::cout << "  search/s <key>            - Search for a key" << std::endl;
    std::cout << "  scan <key1> <key2>        - Range scan from key1 to key2" << std::endl;
    std::cout << "  delete/d <key>            - Delete a key" << std::endl;
    std::cout << "  size                      - Show current memtable size" << std::endl;
    std::cout << "  status                    - Show database status" << std::endl;
    std::cout << "  lsm                       - Show LSM tree structure" << std::endl;
    std::cout << "  compact <level>           - Manually compact a level" << std::endl;
    std::cout << "  searchmode <btree|binary> - Set search mode (btree or binary)" << std::endl;
    std::cout << "  workmode                  - Show current verbose mode status" << std::endl;
    std::cout << "  help                      - Show this help message" << std::endl;
    std::cout << "  clear                     - Clear the console screen" << std::endl;
    std::cout << "  exit                      - Exit the program" << std::endl;
    std::cout << "===================================" << std::endl;
}

void printStatus(const Database& db) {
    std::cout << "\n--- Database Status ---" << std::endl;
    std::cout << "  Database: " << (db.is_open() ? db.get_database_name() : "Not open") << std::endl;
    std::cout << "  Status: " << (db.is_open() ? "Open" : "Closed") << std::endl;
    if (db.is_open()) {
        std::cout << "  Memtable size: " << db.get_size() << "/" << db.get_max_elements() << std::endl;
        double fill_percent = (db.get_max_elements() > 0) 
            ? (db.get_size() * 100.0 / db.get_max_elements()) 
            : 0.0;
        std::cout << "  Fill level: " << std::fixed << std::setprecision(1) << fill_percent << "%" << std::endl;
    }
    std::cout << "  Search mode: " << (db.getUseBTreeSearch() ? "B-Tree" : "Binary") << std::endl;
    #if VERBOSE_MODE
        std::cout << "  Verbose mode: Enabled (for max performance, disable in DBConfig.hpp)" << std::endl;
    #else
        std::cout << "  Verbose mode: Disabled (maximum performance)" << std::endl;
    #endif
    std::cout << "----------------------" << std::endl;
}

int main() {
    // Create database with:
    // - Memtable capacity: 10 key-value pairs
    // - Bloom filter: 10 bits per entry
    // - Bloom filter: 3 hash functions
    Database db(10, 10, 3);
    std::string line, command;
    
    std::cout << "\n╔═══════════════╗" << std::endl;
    std::cout <<   "║   SandBoxDB   ║" << std::endl;
    std::cout <<   "╚═══════════════╝" << std::endl;
    std::cout << "Type 'help' for available commands\n" << std::endl;

    while (true) {
        std::cout << "kv-store> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);
        iss >> command;

        try {
            if (command == "help") {
                printHelp();
            }
            else if (command == "exit" || command == "quit") {
                std::cout << "Closing database and exiting..." << std::endl;
                if (db.is_open()) {
                    db.close_database();
                }
                std::cout << "Goodbye!" << std::endl;
                break;
            }
            else if (command == "open") {
                std::string dbName;
                if (!(iss >> dbName)) {
                    std::cout << "Error: Please provide a database name" << std::endl;
                    std::cout << "Usage: open <db_name>" << std::endl;
                    continue;
                }

                if (db.is_open()) {
                    std::cout << "Closing current database first..." << std::endl;
                    db.close_database();
                }

                if (db.open_database(dbName)) {
                    std::cout << "✓ Database '" << dbName << "' opened successfully" << std::endl;
                } else {
                    std::cout << "✗ Failed to open database '" << dbName << "'" << std::endl;
                }
            }
            else if (command == "close") {
                if (!db.is_open()) {
                    std::cout << "No database is currently open" << std::endl;
                    continue;
                }

                if (db.close_database()) {
                    std::cout << "✓ Database closed successfully" << std::endl;
                } else {
                    std::cout << "✗ Database closed with errors" << std::endl;
                }
            }
            else if (command == "insert" || command == "i") {
                if (!db.is_open()) {
                    std::cout << "Error: No database is open. Use 'open <db_name>' first" << std::endl;
                    continue;
                }

                int key, value;
                if (!(iss >> key >> value)) {
                    std::cout << "Error: Please provide both key and value" << std::endl;
                    std::cout << "Usage: insert <key> <value>" << std::endl;
                    continue;
                }

                if (db.insert(key, value)) {
                    std::cout << "✓ Inserted: " << key << " -> " << value << std::endl;
                } else {
                    std::cout << "✗ Failed to insert key " << key << std::endl;
                }
            }
            else if (command == "seq") {
                if (!db.is_open()) {
                    std::cout << "Error: No database is open. Use 'open <db_name>' first" << std::endl;
                    continue;
                }

                int start, end, step = 1;
                if (!(iss >> start >> end >> step)) {
                    std::cout << "Error: Please provide both start and end values" << std::endl;
                    std::cout << "Usage: seq <start> <end> <step>" << std::endl;
                    continue;
                }

                if (start > end) {
                    std::cout << "Error: Start value must be less than or equal to end value" << std::endl;
                    continue;
                }

                bool allInserted = true;
                for (int k = start; k <= end; k += step) {
                    if (!db.insert(k, k)) {
                        std::cout << "✗ Failed to insert key " << k << std::endl;
                        allInserted = false;
                    }
                }

                if (allInserted) {
                    std::cout << "✓ Inserted sequential keys from " << start << " to " << end << std::endl;
                }
            }
            else if (command == "search" || command == "s") {
                if (!db.is_open()) {
                    std::cout << "Error: No database is open. Use 'open <db_name>' first" << std::endl;
                    continue;
                }

                int key, value;
                if (!(iss >> key)) {
                    std::cout << "Error: Please provide a key to search" << std::endl;
                    std::cout << "Usage: search <key>" << std::endl;
                    continue;
                }

                if (!db.search(key, value)) {
                    std::cout << "✗ Key " << key << " not found" << std::endl;
                } else if (value == -1)
                {
                    std::cout << "✓ Key " << key << " is marked as deleted" << std::endl;
                }
                 else {
                    std::cout << "✓ Found: " << key << " -> " << value << std::endl;
                }
            }
            else if (command == "scan") {
                if (!db.is_open()) {
                    std::cout << "Error: No database is open. Use 'open <db_name>' first" << std::endl;
                    continue;
                }

                int key1, key2;
                if (!(iss >> key1 >> key2)) {
                    std::cout << "Error: Please provide both start and end keys" << std::endl;
                    std::cout << "Usage: scan <key1> <key2>" << std::endl;
                    continue;
                }

                auto results = db.range_scan(key1, key2);
                if (results.empty()) {
                    std::cout << "No entries found in range [" << key1 << ", " << key2 << "]" << std::endl;
                } else {
                    std::cout << "Found " << results.size() << " entries:" << std::endl;
                    std::cout << "┌──────────┬──────────┐" << std::endl;
                    std::cout << "│   Key    │  Value   │" << std::endl;
                    std::cout << "├──────────┼──────────┤" << std::endl;
                    int count = 0;
                    for (const auto& [k, v] : results) {
                        if (v == -1) {
                            continue;
                        }
                        count++;
                        std::cout << "│ " << std::setw(8) << k << " │ " 
                                  << std::setw(8) << v << " │" << std::endl;
                    }
                    std::cout << "└──────────┴──────────┘" << std::endl;

                    if (count == 0) {
                        std::cout << "All keys in the range are marked as deleted." << std::endl;
                    }
                }
            }
            else if (command == "delete" || command == "d") {
                if (!db.is_open()) {
                    std::cout << "Error: No database is open. Use 'open <db_name>' first" << std::endl;
                    continue;
                }

                int key;
                if (!(iss >> key)) {
                    std::cout << "Error: Please provide a key to delete" << std::endl;
                    std::cout << "Usage: delete <key>" << std::endl;
                    continue;
                }

                int value;
                if (!db.search(key, value)) {
                    std::cout << "✗ Key " << key << " not found for deletion" << std::endl;
                }

                if (db.insert(key, -1)) { // Using -1 as a tombstone value
                    std::cout << "✓ Deleted key " << key << std::endl;
                } else {
                    std::cout << "✗ Key " << key << " not found for deletion" << std::endl;
                }
            }
            else if (command == "size") {
                if (!db.is_open()) {
                    std::cout << "Error: No database is open. Use 'open <db_name>' first" << std::endl;
                    continue;
                }
                std::cout << "Memtable size: " << db.get_size() << " / " 
                          << db.get_max_elements() << std::endl;
            }
            else if (command == "status") {
                printStatus(db);
            }
            else if (command == "lsm") {
                if (!db.is_open()) {
                    std::cout << "Error: No database is open. Use 'open <db_name>' first" << std::endl;
                    continue;
                }
                db.print_lsm_structure();
            }
            else if (command == "compact") {
                if (!db.is_open()) {
                    std::cout << "Error: No database is open. Use 'open <db_name>' first" << std::endl;
                    continue;
                }

                int level;
                if (!(iss >> level)) {
                    std::cout << "Error: Please provide a level number" << std::endl;
                    std::cout << "Usage: compact <level>" << std::endl;
                    continue;
                }

                if (db.compact_level(level)) {
                    std::cout << "✓ Level " << level << " compacted successfully" << std::endl;
                } else {
                    std::cout << "✗ Failed to compact level " << level << std::endl;
                }
            }
            else if (command == "searchmode") {
                std::string mode;
                if (!(iss >> mode)) {
                    std::cout << "Error: Please specify search mode (btree or binary)" << std::endl;
                    std::cout << "Usage: searchmode <btree|binary>" << std::endl;
                    std::cout << "Current mode: " << (db.getUseBTreeSearch() ? "btree" : "binary") << std::endl;
                    continue;
                }

                if (mode == "btree") {
                    db.setUseBTreeSearch(true);
                    std::cout << "✓ Search mode set to B-Tree" << std::endl;
                } else if (mode == "binary") {
                    db.setUseBTreeSearch(false);
                    std::cout << "✓ Search mode set to Binary Search" << std::endl;
                } else {
                    std::cout << "Error: Invalid search mode '" << mode << "'" << std::endl;
                    std::cout << "Valid modes: btree, binary" << std::endl;
                }
            }
            else if (command == "workmode") {
                std::cout << "\n--- Verbose Mode Status ---" << std::endl;
                #if VERBOSE_MODE
                    std::cout << "  Current mode: VERBOSE (debug prints enabled)" << std::endl;
                    std::cout << "  Performance: Standard" << std::endl;
                    std::cout << "\n  To disable verbose output for maximum performance:" << std::endl;
                    std::cout << "  1. Open DBConfig.hpp" << std::endl;
                    std::cout << "  2. Change: #define VERBOSE_MODE 1  →  #define VERBOSE_MODE 0" << std::endl;
                    std::cout << "  3. Recompile: make clean && make" << std::endl;
                #else
                    std::cout << "  Current mode: SILENT (debug prints disabled)" << std::endl;
                    std::cout << "  Performance: Maximum" << std::endl;
                    std::cout << "\n  To enable verbose output for debugging:" << std::endl;
                    std::cout << "  1. Open DBConfig.hpp" << std::endl;
                    std::cout << "  2. Change: #define VERBOSE_MODE 0  →  #define VERBOSE_MODE 1" << std::endl;
                    std::cout << "  3. Recompile: make clean && make" << std::endl;
                #endif
                std::cout << "---------------------------" << std::endl;
            }
            else if (command == "clear") {
                // Clear the console screen
                #ifdef _WIN32
                    system("cls");
                #else
                    system("clear");
                #endif
            }
            else {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Type 'help' for available commands" << std::endl;
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
        }
    }

    return 0;
}