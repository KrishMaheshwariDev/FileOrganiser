#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <filesystem>
#include <memory>

#include "SearchManager.h"

// Forward declaration
class FileData;

/**
 * TagManager
 * -----------
 * Manages tags for files indexed by SearchManager.
 *
 * Responsibilities:
 *  - Create / delete tags (persisted in JSON)
 *  - Assign / remove tags from files
 *  - Maintain in-memory mapping of tag → file indices
 *  - Validate / auto-create destination directories for each tag
 *  - Load and save tags.json on startup/shutdown
 *
 * Persistence format (tags.json):
 * {
 *   "tags": {
 *     "game": { "destination": "C:/Projects/Sorted/Game" },
 *     "art":  { "destination": "C:/Projects/Sorted/Art"  }
 *   }
 * }
 */
class TagManager
{
public:
    explicit TagManager(SearchManager &searchManager);
    ~TagManager();

    // ------------------ Tag lifecycle ------------------

    /**
     * Create a new tag with a validated destination path.
     * If the path does not exist, it will be created.
     * Returns true on success or if the tag already exists with same name.
     */
    bool CreateTag(const std::string tagName);

    /**
     * Delete a tag and remove all file associations.
     * Removes its metadata from tags.json but does not delete the folder.
     */
    bool DeleteTag(const std::string tagName);

    // ------------------ Tag assignments ------------------

    /**
     * Assign tag to a file using its filesystem path.
     * Returns false if file cannot be resolved via SearchManager.
     */
    bool AssignTag(const std::filesystem::path &filePath, const std::string &tagName);

    /**
     * Remove all tags from a file using its path.
     */
    bool RemoveTag(const std::filesystem::path &filePath);

    /**
     * Assign tag directly by file index (faster than path lookup).
     */
    bool AssignTagByIndex(size_t fileIndex, const std::string &tagName);

    /**
     * Remove all tags from file by its index.
     */
    bool RemoveTagByIndex(size_t fileIndex);

    // ------------------ Query operations ------------------

    /**
     * Get all FileData* that have a given tag.
     * Returns empty vector if tag not found or no matches.
     */
    std::vector<FileData *> GetFilesByTag(const std::string &tagName);

    /**
     * Return internal map of tags and associated file indices.
     * NOTE: for inspection only, modifying this map directly is unsafe.
     */
    const std::unordered_map<std::string, std::vector<size_t>> &GetTagMap() const;

    // ------------------ Utility ------------------

    /**
     * Normalize a tag name for internal storage.
     * Currently identity; modify here if you later want case-insensitive behavior.
     */
    static std::string NormalizeTag(const std::string &tag);

private:
    SearchManager &m_searchManager;

    // pimpl to hide TagInfo structure and reduce header rebuilds
    class Impl;
    std::unique_ptr<Impl> m_impl;

    // Cache for GetTagMap() view
    mutable std::unordered_map<std::string, std::vector<size_t>> m_tagMapCache;
    mutable bool m_tagMapCacheInitialized = false;

    // ------------------ Internal Helpers ------------------
    bool SaveTagsToJson() const;
    bool LoadTagsFromJson();
    bool ValidateDestination(const std::string &path, std::string &outAbsolute) const;

    // Resolve file index by path using SearchManager (adapt to your API)
    std::optional<size_t> ResolveFileIndex(const std::filesystem::path &filePath) const;
};
