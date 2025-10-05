#ifndef FILE_OPERATIONS_HPP
#define FILE_OPERATIONS_HPP

#include <string>
#include <vector>
#include <utility>

class FileOperations {
public:
    // Directory operations
    static bool create_directory(const std::string& path);
    static bool directory_exists(const std::string& dirName);
    
    // File operations
    static bool file_exists(const std::string& fileName);
    static std::vector<std::pair<int, int>> read_sst_file(const std::string& filePath);
    static bool write_sst_file(const std::vector<std::pair<int, int>>& data, 
                              const std::string& filepath, bool isAtomic = true);
    
    // SST file management
    static int count_sst_files(const std::string& directory);
    static bool remove_file(const std::string& filePath);
};

#endif // FILE_OPERATIONS_HPP