#include "Database.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <limits>
#include <iomanip>

void printHelp() {
    std::cout << "\n=== KV-Store Database Commands ===" << std::endl;
    std::cout << "  open <db_name>          - Open/create a database" << std::endl;
    std::cout << "  close                   - Close current database" << std::endl;
    std::cout << "  insert <key> <value>    - Insert a key-value pair" << std::endl;
    std::cout << "  search <key>            - Search for a key" << std::endl;
    std::cout << "  scan <key1> <key2>      - Range scan from key1 to key2" << std::endl;
    std::cout << "  size                    - Show current memtable size" << std::endl;
    std::cout << "  status                  - Show database status" << std::endl;
    std::cout << "  lsm                     - Show LSM tree structure" << std::endl;
    std::cout << "  compact <level>         - Manually compact a level" << std::endl;
    std::cout << "  help                    - Show this help message" << std::endl;
    std::cout << "  exit                    - Exit the program" << std::endl;
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
            else if (command == "insert") {
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
            else if (command == "search") {
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

                if (db.search(key, value)) {
                    std::cout << "✓ Found: " << key << " -> " << value << std::endl;
                } else {
                    std::cout << "✗ Key " << key << " not found" << std::endl;
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
                    for (const auto& [k, v] : results) {
                        std::cout << "│ " << std::setw(8) << k << " │ " 
                                  << std::setw(8) << v << " │" << std::endl;
                    }
                    std::cout << "└──────────┴──────────┘" << std::endl;
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