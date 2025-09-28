#include "Memtable_ds.hpp"
#include <iostream>
// #include <unordered_map>
// #include <chrono>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
// #include <dirent.h>
// #include <cstdio>
// #include <unordered_set>
// #include <algorithm>
using namespace std;

// #include <stdio.h>

class Database
{
private:
    Memtable_ds *engine;
    std::string databaseName;
    std::string databaseDirectory; // underlying storage engine (AVL + SST)
    bool isOpen;

public:
    Database(int memtableCapacity = 1000)
    {
        // engine = new AVL(memtableCapacity);
        std::unique_ptr<Memtable_ds> engine = create_memtable(memtableCapacity);
        isOpen = false;
    }

    ~Database()
    {
        if (isOpen)
        {
            closeDatabase();
        }
        delete engine;
    }

    bool openDatabase(const std::string &dbName)
    {
        if (isOpen)
        {
            std::cout << "Database already open!" << std::endl;
            return false;
        }

        /*
            Sets database name and constructs directory path.
            Creates directory if it doesn't exist using create_directory.
            Initializes empty memtable for new operations.
        */
        cout << "Opening database: " << dbName << endl;

        // Validate database name
        if (dbName.empty())
        {
            cout << "Error: Database name cannot be empty" << endl;
            return false;
        }

        // Check for invalid characters in database name (basic validation)
        for (char c : dbName)
        {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            {
                cout << "Error: Database name contains invalid characters: " << dbName << endl;
                return false;
            }
        }

        // Set database name and construct directory path
        engine->setDatabaseName(dbName);
        engine->setDatabaseDirectory(dbName); // Simple approach: directory name = database name

        // Create directory if it doesn't exist
        if (!create_directory(databaseDirectory))
        {
            cout << "Failed to create or access database directory: " << databaseDirectory << endl;
            databaseName.clear();
            databaseDirectory.clear();
            return false;
        }

        // Initialize empty memtable for new operations
        // Clear any existing data first
        engine->clear_memtable();
        isOpen = true;
        reload_incomplete_sst();
        cout << "Successfully opened database: " << databaseName << " at directory: " << databaseDirectory << endl;

        return true;
    }

    bool closeDatabase()
    {
        /*
      On close, if memtable has data, flush to SST as usual,
      then rename the SST file with the highest number to incomplete.txt.
      Cleans up resources and resets state.
  */
        cout << "Closing database: " << databaseName << endl;

        if (!isOpen)
        {
            std::cout << "No database is currently open" << std::endl;
            return false;
        }
        if (databaseName.empty())
        {
            cout << "No database is currently open" << endl;
            return true; // Not an error, just nothing to close
        }

        bool success = true;

        // Check if memtable has data using get_size()
        if (engine->get_size() > 0)
        {
            cout << "Memtable contains " << engine->get_size() << " entries, flushing to SST before close" << endl;

            // Flush to SST
            if (!engine->flush_to_sst())
            {
                cout << "Error: Failed to flush memtable data during database close" << endl;
                success = false;
            }
            else
            {
                cout << "Successfully flushed memtable data to SST file" << endl;

                // Find the SST file with the highest number
                int maxNum = -1;
                std::string maxFile;
                DIR *dir = opendir(databaseDirectory.c_str());
                if (dir != nullptr)
                {
                    struct dirent *entry;
                    while ((entry = readdir(dir)) != nullptr)
                    {
                        std::string filename = entry->d_name;
                        if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".txt")
                        {
                            std::string numberPart = filename.substr(0, filename.length() - 4);
                            bool isNumber = !numberPart.empty() && std::all_of(numberPart.begin(), numberPart.end(), ::isdigit);
                            if (isNumber)
                            {
                                int num = std::stoi(numberPart);
                                if (num > maxNum)
                                {
                                    maxNum = num;
                                    maxFile = filename;
                                }
                            }
                        }
                    }
                    closedir(dir);
                }
                // Rename last SST file to incomplete.txt
                if (!maxFile.empty())
                {
                    std::string oldPath = databaseDirectory + "/" + maxFile;
                    std::string newPath = databaseDirectory + "/incomplete.txt";
                    if (std::rename(oldPath.c_str(), newPath.c_str()) == 0)
                    {
                        cout << "Renamed " << maxFile << " to incomplete.txt" << endl;
                    }
                    else
                    {
                        cout << "Failed to rename " << maxFile << " to incomplete.txt" << endl;
                        success = false;
                    }
                }
            }
        }
        else
        {
            cout << "Memtable is empty, no data to flush" << endl;
        }

        // Clean up resources and reset state
        engine->clear_memtable();
        databaseName.clear();
        databaseDirectory.clear();

        if (success)
        {
            cout << "Database closed successfully" << endl;
        }
        else
        {
            cout << "Database closed with errors (data may have been lost)" << endl;
        }

        return success;
    }

    void reload_incomplete_sst()
    {
        // Go through all files in the database directory
        DIR *dir = opendir(databaseDirectory.c_str());
        if (dir == nullptr)
        {
            std::cout << "Error opening directory: " << databaseDirectory << std::endl;
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            std::string filename = entry->d_name;
            if (filename == "incomplete.txt")
            {
                std::string incompletePath = databaseDirectory + "/incomplete.txt";
                std::ifstream file(incompletePath);
                if (!file.is_open())
                {
                    std::cout << "Failed to open incomplete.txt for reload" << std::endl;
                    continue;
                }
                int key, value;
                while (file >> key >> value)
                {
                    engine->insert(key, value);
                }
                file.close();

                // Delete incomplete.txt after reload
                if (std::remove(incompletePath.c_str()) == 0)
                {
                    std::cout << "Reloaded and deleted incomplete.txt" << std::endl;
                }
                else
                {
                    std::cout << "Failed to delete incomplete.txt after reload" << std::endl;
                }
            }
        }
        closedir(dir);
    }

    bool create_directory(const std::string &path)
    {
        /*
            Function to create database directory if it doesn't exist.
            Handles directory creation errors gracefully.
            Uses POSIX directory creation approach.
        */
        try
        {
            // Check if directory already exists
            struct stat st;
            if (stat(path.c_str(), &st) == 0)
            {
                if (S_ISDIR(st.st_mode))
                {
                    cout << "Directory already exists: " << path << endl;
                    return true;
                }
                else
                {
                    cout << "Path exists but is not a directory: " << path << endl;
                    return false;
                }
            }

            // Create directory with permissions 0755
            if (mkdir(path.c_str(), 0755) == 0)
            {
                cout << "Successfully created directory: " << path << endl;
                return true;
            }
            else
            {
                cout << "Failed to create directory: " << path << endl;
                return false;
            }
        }
        catch (const std::exception &ex)
        {
            cout << "Unexpected error creating directory " << path << ": " << ex.what() << endl;
            return false;
        }
    }

    bool put(int key, int value)
    {
        if (!isOpen)
        {
            std::cout << "Error: Database not open" << std::endl;
            return false;
        }
        return engine->insert(key, value) != nullptr;
    }

    bool get(int key, int &value)
    {
        if (!isOpen)
        {
            std::cout << "Error: Database not open" << std::endl;
            return false;
        }
        return engine->get(key, value);
    }

    std::vector<std::pair<int, int>> scan(int key1, int key2)
    {
        if (!isOpen)
        {
            std::cout << "Error: Database not open" << std::endl;
            return {};
        }
        return engine->range_scan_with_sst(key1, key2);
    }
};
