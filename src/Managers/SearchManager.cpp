#include "SearchManager.h"
#include <iostream>

namespace fs = std::filesystem;

SearchManager::SearchManager(SearchMode mode)
    : m_NextFileID(0), m_lastMode(mode)
{
}

bool SearchManager::LoadMetaData(const std::string &path, SearchMode mode)
{
    fs::path filePath = path;

    currentDirectoryPath = filePath;
    m_lastMode = mode;
    m_NextFileID = 0;
    m_files.clear();
    m_filePathIndexMap.clear();

    if (mode == SearchMode::TOP_LEVEL)
    {
        for (const auto &entry : fs::directory_iterator(filePath))
        {

            FileData file = getFileData(entry);
            if (file.fileID == -1)
            {
                return false;
            }
            file.fileID = m_NextFileID;
            m_NextFileID++;

            // pushing the actual file data to vector
            m_files.push_back(file);
            // pushing file ID as key and its index to value (for fast look ups in the vector)
            m_filePathIndexMap[file.path.string()] = m_files.size() - 1;
        }

        return true;
    }
    else if (mode == SearchMode::RECURSIVE)
    {
        for (const auto &entry : fs::recursive_directory_iterator(filePath))
        {
            FileData file = getFileData(entry);
            if (file.fileID == -1)
            {
                return false;
            }
            file.fileID = m_NextFileID;
            m_NextFileID++;

            // pushing the actual file data to vector
            m_files.push_back(file);
            // pushing file ID as key and its index to value (for fast look ups in the vector)
            m_filePathIndexMap[file.path.string()] = m_files.size() - 1;
        }
        return true;
    }
    else
    {
        std::cout << "Unexpected error occured in mode search";
        return false;
    }
}

FileData SearchManager::getFileData(const fs::directory_entry &entry)
{
    FileData file;
    try
    {
        // setting File ID
        // file.fileID = m_NextFileID;
        // m_NextFileID++;

        // Setting File Name
        file.name = entry.path().stem().string();

        // Setting File path
        file.path = entry.path();

        // Setting File type
        if (fs::is_regular_file(entry.path()))
        {
            file.type = FileType::REGULAR_FILE;
        }
        else if (fs::is_directory(entry.path()))
        {
            file.type = FileType::DIRECTORY;
        }
        else if (fs::is_symlink(entry.path()))
        {
            file.type = FileType::SYMBOLIC_LINK;
        }
        else
        {
            file.type = FileType::MISC;
        }

        // Setting modified time

        fs::file_time_type lastWriteTime = fs::last_write_time(entry.path());

        std::time_t tt = std::chrono::system_clock::to_time_t(
            std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                lastWriteTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()));

        file.modifiedTime = std::chrono::system_clock::from_time_t(tt);

        return file;
    }
    catch (const fs::filesystem_error &e)
    {
        std::cout << "Error accessing file: " << e.what() << std::endl;
        file.fileID = -1;
        return file;
    }
}

bool SearchManager::Refresh()
{
    const bool isRecursive = (m_lastMode == SearchMode::RECURSIVE);

    try
    {

        if (isRecursive)
        {
            for (const auto &entry : fs::recursive_directory_iterator(currentDirectoryPath))
            {
                auto pathStr = entry.path().string();
                auto it = m_filePathIndexMap.find(pathStr);

                if (it != m_filePathIndexMap.end())
                {
                    FileData refreshFile = getFileData(entry);
                    auto &stored = m_files[it->second];

                    if (refreshFile.modifiedTime != stored.modifiedTime)
                    {
                        refreshFile.fileID = stored.fileID;
                        stored = refreshFile;
                    }
                }
                else
                {
                    FileData file = getFileData(entry);
                    if (file.fileID == -1)
                        return false;

                    file.fileID = m_NextFileID++;
                    m_files.push_back(file);
                }
            }
        }
        else
        {
            for (const auto &entry : fs::directory_iterator(currentDirectoryPath))
            {
                auto pathStr = entry.path().string();
                auto it = m_filePathIndexMap.find(pathStr);

                if (it != m_filePathIndexMap.end())
                {
                    FileData refreshFile = getFileData(entry);
                    auto &stored = m_files[it->second];

                    if (refreshFile.modifiedTime != stored.modifiedTime)
                    {
                        refreshFile.fileID = stored.fileID;
                        stored = refreshFile;
                    }
                }
                else
                {
                    FileData file = getFileData(entry);
                    if (file.fileID == -1)
                        return false;

                    file.fileID = m_NextFileID++;
                    m_files.push_back(file);
                }
            }
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cout << "Error refreshing directory: " << e.what() << std::endl;
        return false;
    }

    return true;
}

const std::vector<FileData> &SearchManager::GetAllFiles() const
{
    return m_files;
}

const FileData *SearchManager::FindFileByID(int id) const
{
    for (auto &file : m_files)
    {
        if (file.fileID == id)
        {
            return &file;
        }
    }
    return nullptr;
}

FileData *SearchManager::FindFileByID(int id)
{
    for (auto &file : m_files)
    {
        if (file.fileID == id)
        {
            return &file;
        }
    }
    return nullptr;
}

const FileData *SearchManager::FindFileByName(const std::string &name) const
{
    for (auto &file : m_files)
    {
        if (file.name == name)
        {
            return &file;
        }
    }
    return nullptr;
}

FileData *SearchManager::FindFileByName(const std::string &name)
{
    for (auto &file : m_files)
    {
        if (file.name == name)
        {
            return &file;
        }
    }
    return nullptr;
}

const FileData *SearchManager::FindFileByPath(const fs::path &path) const
{
    auto it = m_filePathIndexMap.find(path.string());
    if (it == m_filePathIndexMap.end())
        return nullptr;
    return &m_files[it->second];
}

FileData *SearchManager::FindFileByPath(const fs::path &path)
{
    auto it = m_filePathIndexMap.find(path.string());
    if (it == m_filePathIndexMap.end())
        return nullptr;
    return &m_files[it->second];
}