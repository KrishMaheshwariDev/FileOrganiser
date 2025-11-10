#include "FileManager.h"
#include "../../include/json/json.hpp"
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

FileManager::FileManager(TagManager &tagManager, SearchManager &searchManager)
    : m_tagManager(tagManager), m_searchManager(searchManager)
{
}

size_t FileManager::MoveAllTaggedFiles()
{
    size_t movedCount = 0;
    const auto &tagMap = m_tagManager.GetTagMap();

    for (const auto &[tagName, fileIndices] : tagMap)
    {
        movedCount += MoveFilesByTag(tagName);
    }

    return movedCount;
}

size_t FileManager::MoveFilesByTag(const std::string &tagName)
{
    size_t movedCount = 0;
    const auto &tagMap = m_tagManager.GetTagMap();
    auto it = tagMap.find(tagName);
    if (it == tagMap.end())
        return 0;

    // We’ll need destination path from TagManager internals
    // Adapted since TagManager stores it inside Impl
    // To keep decoupling, we’ll use GetFilesByTag() instead
    std::vector<FileData *> files = m_tagManager.GetFilesByTag(tagName);

    // Reopen TagManager JSON to fetch destination
    std::ifstream ifs("tags.json");
    if (!ifs.is_open())
    {
        std::cerr << "FileManager: cannot read tags.json.\n";
        return 0;
    }

    nlohmann::json j;
    ifs >> j;
    ifs.close();

    if (!j.contains("tags") || !j["tags"].contains(tagName))
        return 0;

    std::string destination = j["tags"][tagName]["destination"].get<std::string>();
    EnsureDirectory(destination);

    for (FileData *fd : files)
    {
        if (!fd)
            continue;
        if (MoveSingleFile(*fd, destination))
            movedCount++;
    }

    return movedCount;
}

bool FileManager::MoveSingleFile(const FileData &file, const std::string &destination)
{
    try
    {
        fs::path src = file.path;
        if (!fs::exists(src))
        {
            std::cerr << "FileManager: missing source " << src << "\n";
            return false;
        }

        fs::path destDir(destination);
        EnsureDirectory(destDir);

        fs::path destFile = destDir / src.filename();

        // Handle conflict: rename duplicate files
        if (fs::exists(destFile))
        {
            std::string stem = destFile.stem().string();
            std::string ext = destFile.extension().string();
            int count = 1;
            while (fs::exists(destFile))
            {
                destFile = destDir / fs::path(stem + "_" + std::to_string(count++) + ext);
            }
        }

        fs::rename(src, destFile);

        std::cout << "Moved: " << src << " -> " << destFile << "\n";
        return true;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "FileManager::MoveSingleFile error: " << ex.what() << "\n";
        return false;
    }
}

void FileManager::EnsureDirectory(const fs::path &dest)
{
    std::error_code ec;
    if (!fs::exists(dest, ec))
        fs::create_directories(dest, ec);
}
