#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <filesystem>

enum class SearchMode
{
    TOP_LEVEL,
    RECURSIVE
};

enum class FileType
{
    REGULAR_FILE,
    DIRECTORY,
    SYMBOLIC_LINK,
    MISC
};

struct FileData
{
    int fileID;
    std::string name;
    std::filesystem::path path;
    FileType type;
    std::string tag;
    std::chrono::system_clock::time_point modifiedTime;
};

class SearchManager
{
public:
    SearchManager(SearchMode mode);

    bool LoadMetaData(const std::string &directoryPath, SearchMode mode);
    bool Refresh();

    const std::vector<FileData> &GetAllFiles() const;

    const FileData *FindFileByID(int id) const; // for immutable data
    FileData *FindFileByID(int id);             // for tag marking

    const FileData *FindFileByPath(const std::filesystem::path &path) const;
    FileData *FindFileByPath(const std::filesystem::path &path);

    const FileData *FindFileByName(const std::string &name) const;
    FileData *FindFileByName(const std::string &name);

private:
    std::vector<FileData> m_files;
    std::unordered_map<std::string, std::size_t> m_filePathIndexMap;

    int m_NextFileID;
    SearchMode m_lastMode;
    std::filesystem::path currentDirectoryPath;

    // utils method
    FileData getFileData(const std ::filesystem::directory_entry &entry);
};