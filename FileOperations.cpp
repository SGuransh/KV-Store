#include "FileOperations.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>

bool FileOperations::create_directory(const std::string& path) {
    try {
        // Check if directory already exists
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                std::cout << "Directory already exists: " << path << std::endl;
                return true;
            } else {
                std::cout << "Path exists but is not a directory: " << path << std::endl;
                return false;
            }
        }

        // Create directory with permissions 0755
        if (mkdir(path.c_str(), 0755) == 0) {
            std::cout << "Successfully created directory: " << path << std::endl;
            return true;
        } else {
            std::cout << "Failed to create directory: " << path << std::endl;
            return false;
        }

    } catch (const std::exception& ex) {
        std::cout << "Unexpected error creating directory " << path << ": " << ex.what() << std::endl;
        return false;
    }
}

bool FileOperations::directory_exists(const std::string& dirName) {
    struct stat st;
    return (stat(dirName.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
}

bool FileOperations::file_exists(const std::string& fileName) {
    struct stat st;
    return (stat(fileName.c_str(), &st) == 0) && S_ISREG(st.st_mode);
}

std::vector<std::pair<int, int>> FileOperations::read_sst_file(const std::string& filePath) {
    std::vector<std::pair<int, int>> content;
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Failed to open file: " << filePath << std::endl;
        return content;
    }
    
   int key, value;
    while (file.read(reinterpret_cast<char*>(&key), sizeof(int)) &&
           file.read(reinterpret_cast<char*>(&value), sizeof(int))) {
        content.push_back({key, value});
    }
    file.close();
    return content;
}

bool FileOperations::write_sst_file(const std::vector<std::pair<int, int>>& data, 
                                   const std::string& filepath, bool isAtomic) {
    try {
        std::string actualPath = filepath;
        std::string tempPath = filepath + ".tmp";
        
        // Choose write path based on atomic flag
        std::string writePath = isAtomic ? tempPath : actualPath;
        
        std::ofstream file(writePath, std::ios::binary);
        if (!file.is_open()) {
            std::cout << "Failed to create SST file: " << writePath << std::endl;
            return false;
        }

        // Write data in simple text format: "key value" per line
          for (const auto& pair : data) {
            file.write(reinterpret_cast<const char*>(&pair.first), sizeof(int));
            file.write(reinterpret_cast<const char*>(&pair.second), sizeof(int));
        }

        file.close();

        // Check if write was successful
        if (file.fail()) {
            std::cout << "Failed to write data to SST file" << std::endl;
            if (isAtomic) {
                std::remove(tempPath.c_str());
            }
            return false;
        }

        // Atomically rename temp file to final file if using atomic writes
        if (isAtomic) {
            if (std::rename(tempPath.c_str(), actualPath.c_str()) != 0) {
                std::cout << "Failed to rename temporary file to final file" << std::endl;
                std::remove(tempPath.c_str());
                return false;
            }
        }
        
        std::cout << "Successfully wrote SST file: " << actualPath << " with " << data.size() << " entries" << std::endl;
        return true;

    } catch (const std::exception& ex) {
        std::cout << "Error writing SST file: " << ex.what() << std::endl;
        return false;
    }
}

int FileOperations::count_sst_files(const std::string& directory) {
    if (directory.empty()) {
        return 0;
    }

    // Check if directory exists using stat
    struct stat st;
    if (stat(directory.c_str(), &st) != 0) {
        return 0;
    }

    int count = 0;
    DIR* dir = opendir(directory.c_str());
    if (dir == nullptr) {
        std::cout << "Error opening directory: " << directory << std::endl;
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;
        // Check if file matches SST pattern (number.txt) - exclude incomplete.txt
        if (filename != "incomplete.txt" && filename.length() > 4 && 
            filename.substr(filename.length() - 4) == ".txt") {
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

bool FileOperations::remove_file(const std::string& filePath) {
    return std::remove(filePath.c_str()) == 0;
}