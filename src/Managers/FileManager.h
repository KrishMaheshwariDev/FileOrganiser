#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "TagManager.h"
#include "SearchManager.h"

class FileManager
{
public:
    FileManager(TagManager &tagManager, SearchManager &searchManager);

    /**
     * Move all tagged files to their respective destination directories.
     * Called when user presses “Move” in the GUI.
     * Returns number of successfully moved files.
     */
    size_t MoveAllTaggedFiles();

    /**
     * Move only files for a specific tag.
     */
    size_t MoveFilesByTag(const std::string &tagName);

private:
    TagManager &m_tagManager;
    SearchManager &m_searchManager;

    bool MoveSingleFile(const FileData &file, const std::string &destination);
    void EnsureDirectory(const std::filesystem::path &dest);
};
