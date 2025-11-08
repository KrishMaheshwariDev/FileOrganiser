// TagManager.cpp
#include "TagManager.h"

#include <filesystem>
#include <fstream>
#include <optional>
#include <iostream> // only for debugging/logging, remove or replace with engine logger
#include <cstdio>   // std::remove / std::rename

// nlohmann json
#include "../../include/json.hpp"
using json = nlohmann::json;

namespace fs = std::filesystem;

static constexpr const char *TAG_JSON_FILENAME = "tags.json";

struct TagInfo
{
    std::string destination;
    std::vector<size_t> fileIndices;
};

// Internal storage: normalized tag -> TagInfo
// We'll expose the original map view through the existing header's GetTagMap signature
// by converting to the expected type when required.
class TagManager::Impl
{
public:
    std::unordered_map<std::string, TagInfo> tags;
};

TagManager::TagManager(SearchManager &searchManager)
    : m_searchManager(searchManager), m_impl(std::make_unique<Impl>())
{
    // Attempt to load existing tags from JSON on construction
    LoadTagsFromJson();
}

TagManager::~TagManager() = default;

// --------------------- Private helper declarations ---------------------
// Note: these helpers are implemented below the public methods

// Persist/load helpers
bool TagManager::SaveTagsToJson() const
{
    try
    {
        json j;
        j["tags"] = json::object();

        for (const auto &pair : m_impl->tags)
        {
            const std::string &tag = pair.first;
            const TagInfo &info = pair.second;

            json tagObj;
            tagObj["destination"] = info.destination;
            // We do not persist file indices (they are runtime associations),
            // but if you prefer to persist associations too, add them here.
            j["tags"][tag] = tagObj;
        }

        // Atomic write: write to temp file then rename
        const std::string tmpName = std::string(TAG_JSON_FILENAME) + ".tmp";
        {
            std::ofstream ofs(tmpName, std::ios::out | std::ios::trunc);
            if (!ofs.is_open())
            {
                // Logging - replace with engine logger if available
                std::cerr << "TagManager: failed to open temp json file for writing: " << tmpName << "\n";
                return false;
            }
            ofs << j.dump(4);
            ofs.close();
        }

        // rename tmp -> final (replace if exists)
        std::error_code ec;
        fs::rename(tmpName, TAG_JSON_FILENAME, ec);
        if (ec)
        {
            // On some platforms rename fails when target exists; try remove+rename
            fs::remove(TAG_JSON_FILENAME, ec); // ignore error
            fs::rename(tmpName, TAG_JSON_FILENAME, ec);
            if (ec)
            {
                std::cerr << "TagManager: failed to finalize json save: " << ec.message() << "\n";
                return false;
            }
        }

        return true;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "TagManager::SaveTagsToJson exception: " << ex.what() << "\n";
        return false;
    }
}

bool TagManager::LoadTagsFromJson()
{
    try
    {
        if (!fs::exists(TAG_JSON_FILENAME))
        {
            // Create an empty json file
            json j;
            j["tags"] = json::object();
            std::ofstream ofs(TAG_JSON_FILENAME, std::ios::out | std::ios::trunc);
            ofs << j.dump(4);
            ofs.close();
            return true;
        }

        std::ifstream ifs(TAG_JSON_FILENAME);
        if (!ifs.is_open())
        {
            std::cerr << "TagManager: failed to open tags.json for reading\n";
            return false;
        }

        json j;
        ifs >> j;
        ifs.close();

        if (!j.contains("tags") || !j["tags"].is_object())
        {
            std::cerr << "TagManager: tags.json missing 'tags' object; reinitializing\n";
            m_impl->tags.clear();
            return SaveTagsToJson();
        }

        for (auto it = j["tags"].begin(); it != j["tags"].end(); ++it)
        {
            const std::string tag = it.key();
            const json &tagObj = it.value();
            TagInfo info;
            if (tagObj.contains("destination") && tagObj["destination"].is_string())
            {
                info.destination = tagObj["destination"].get<std::string>();
            }
            else
            {
                info.destination = "";
            }
            // fileIndices remain empty on load (runtime association)
            info.fileIndices.clear();
            m_impl->tags.emplace(tag, std::move(info));
        }

        return true;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "TagManager::LoadTagsFromJson exception: " << ex.what() << "\n";
        return false;
    }
}

bool TagManager::ValidateDestination(const std::string &path, std::string &outAbsolute) const
{
    try
    {
        fs::path p(path);
        fs::path abs = fs::absolute(p);
        // Try to create directories if they don't exist
        std::error_code ec;
        if (!fs::exists(abs, ec))
        {
            if (!fs::create_directories(abs, ec) || ec)
            {
                std::cerr << "TagManager: failed to create destination directory: " << abs.string()
                          << " (" << ec.message() << ")\n";
                return false;
            }
        }
        else if (!fs::is_directory(abs, ec) || ec)
        {
            std::cerr << "TagManager: destination exists but is not a directory: " << abs.string() << "\n";
            return false;
        }

        outAbsolute = abs.string();
        return true;
    }
    catch (const std::exception &ex)
    {
        std::cerr << "TagManager::ValidateDestination exception: " << ex.what() << "\n";
        return false;
    }
}

// Resolve file index from path using SearchManager.
// NOTE: adapt this to the actual SearchManager API in your project.
// The current implementation tries two common patterns. If they don't match your SearchManager,
// replace the body by the proper call.
std::optional<size_t> TagManager::ResolveFileIndex(const std::filesystem::path &filePath) const
{
    // Attempt 1: SearchManager has a method named GetFileIndex or FindFileIndex which returns optional<size_t>
    // Attempt 2: SearchManager can provide FileData* for a path and that FileData contains an index
    // If neither exists, you must adapt this function to your SearchManager's API.

    // --- BEGIN: ADAPT THIS SECTION IF NECESSARY ---
    // Example assumed APIs (uncomment and adapt if they exist):
    // if (auto idxOpt = m_searchManager.GetFileIndex(filePath); idxOpt.has_value()) return idxOpt;
    // if (auto idxOpt = m_searchManager.FindFileIndex(filePath); idxOpt.has_value()) return idxOpt;

    // Fallback: try to find a FileData* for the path and extract its index
    // Assumes SearchManager::GetFileDataByPath(const path&) -> FileData*
    try
    {
        // The following call likely does not exist verbatim; adapt to real API:
        if constexpr (true)
        {
            // TODO: Replace this pseudo-call with real SearchManager call:
            // FileData* fd = m_searchManager.GetFileDataByPath(filePath);
            // if (fd) return fd->index;
        }
    }
    catch (...)
    {
        // swallow - will return nullopt below
    }

    // If you can't resolve fileIndex here, return nullopt
    return std::nullopt;
}

// Safe add unique index into vector
static void push_unique_index(std::vector<size_t> &vec, size_t idx)
{
    if (std::find(vec.begin(), vec.end(), idx) == vec.end())
    {
        vec.push_back(idx);
    }
}

// --------------------- Public API implementations ---------------------

bool TagManager::CreateTag(const std::string tagName)
{
    const std::string tag = NormalizeTag(tagName);
    if (m_impl->tags.find(tag) != m_impl->tags.end())
    {
        return false; // already exists
    }

    // For create without destination: set empty destination (user may set later)
    TagInfo info;
    info.destination.clear();
    info.fileIndices.clear();
    m_impl->tags.emplace(tag, std::move(info));

    return SaveTagsToJson();
}

bool TagManager::DeleteTag(const std::string tagName)
{
    const std::string tag = NormalizeTag(tagName);
    auto it = m_impl->tags.find(tag);
    if (it == m_impl->tags.end())
    {
        return false;
    }

    // erase mapping (removes tag -> fileIndices association)
    m_impl->tags.erase(it);

    return SaveTagsToJson();
}

bool TagManager::AssignTag(const std::filesystem::path &filePath, const std::string &tagName)
{
    // Resolve file index via SearchManager
    auto idxOpt = ResolveFileIndex(filePath);
    if (!idxOpt.has_value())
    {
        // Failed to find file in search manager
        return false;
    }
    return AssignTagByIndex(idxOpt.value(), tagName);
}

bool TagManager::RemoveTag(const std::filesystem::path &filepath)
{
    auto idxOpt = ResolveFileIndex(filepath);
    if (!idxOpt.has_value())
    {
        return false;
    }
    return RemoveTagByIndex(idxOpt.value());
}

bool TagManager::AssignTagByIndex(size_t fileIndex, const std::string &tagName)
{
    const std::string tag = NormalizeTag(tagName);

    // Ensure tag exists. If not, create with empty destination.
    auto it = m_impl->tags.find(tag);
    if (it == m_impl->tags.end())
    {
        TagInfo info;
        info.destination.clear();
        info.fileIndices.clear();
        it = m_impl->tags.emplace(tag, std::move(info)).first;

        // Persist creation
        if (!SaveTagsToJson())
        {
            // If save failed, remove the inserted tag to keep memory/JSON consistent
            m_impl->tags.erase(tag);
            return false;
        }
    }

    // Insert unique index
    push_unique_index(it->second.fileIndices, fileIndex);
    return true;
}

bool TagManager::RemoveTagByIndex(size_t fileIndex)
{
    bool anyRemoved = false;
    std::vector<std::string> emptyTags;

    for (auto &pair : m_impl->tags)
    {
        auto &vec = pair.second.fileIndices;
        auto oldSize = vec.size();
        vec.erase(std::remove(vec.begin(), vec.end(), fileIndex), vec.end());
        if (vec.size() != oldSize)
        {
            anyRemoved = true;
        }
        // Optionally, if the tag has no more files, we leave it in place (per earlier rules).
        // If you prefer to remove tag entries when empty, collect them:
        // if (vec.empty()) emptyTags.push_back(pair.first);
    }

    // optional: remove tags that have no destinations & no files (not required)
    (void)emptyTags;

    return anyRemoved;
}

std::vector<FileData *> TagManager::GetFilesByTag(const std::string &tagName)
{
    std::vector<FileData *> out;
    const std::string tag = NormalizeTag(tagName);
    auto it = m_impl->tags.find(tag);
    if (it == m_impl->tags.end())
        return out;

    for (size_t idx : it->second.fileIndices)
    {
        // TODO: adapt to your SearchManager's API to fetch FileData* by index
        // Example assumed API:
        // FileData* fd = m_searchManager.GetFileDataByIndex(idx);
        FileData *fd = nullptr;
        if (fd)
            out.push_back(fd);
    }

    return out;
}

const std::unordered_map<std::string, std::vector<size_t>> &TagManager::GetTagMap() const
{
    // Build a temporary view to match header signature would require copying.
    // The header returns a const reference to internal storage type:
    // const std::unordered_map<std::string, std::vector<size_t>> &
    // But we internally store TagInfo. To honor the header without changing it,
    // we maintain a static cache mapping view. Simpler approach is to adapt header
    // to return TagInfo map directly. For now, we create a persistent member-level
    // view to return a reference to. (This is a small shim — if you prefer, change the header.)
    if (!m_tagMapCacheInitialized)
    {
        m_tagMapCache.clear();
        for (const auto &pair : m_impl->tags)
        {
            m_tagMapCache.emplace(pair.first, pair.second.fileIndices);
        }
        m_tagMapCacheInitialized = true;
    }
    return m_tagMapCache;
}

std::string TagManager::NormalizeTag(const std::string &tag)
{
    // Identity implementation as requested. Keep as a single place to change later.
    return tag;
}

// --------------------- Additional private members & initialization ---------------------

// We declare the private members that support GetTagMap cache and the pimpl above.
class TagManagerPrivateMembers
{
public:
    std::unordered_map<std::string, std::vector<size_t>> m_tagMapCache;
    bool initialized = false;
};

// Because header has no place for these, we add them as members to TagManager (C++ allowed if declared in class).
// But since we cannot change header here, we emulate with static globals (not ideal, but keeps compatibility).
// Better: update the header to expose TagInfo map or return a copy.

static std::unordered_map<std::string, std::vector<size_t>> g_tag_map_cache;
static bool g_tag_map_cache_init = false;

// Provide a thin wrapper to keep GetTagMap semantics
const std::unordered_map<std::string, std::vector<size_t>> &TagManager::GetTagMap() const
{
    // Rebuild cache always to reflect up-to-date data
    g_tag_map_cache.clear();
    for (const auto &pair : m_impl->tags)
    {
        g_tag_map_cache.emplace(pair.first, pair.second.fileIndices);
    }
    g_tag_map_cache_init = true;
    return g_tag_map_cache;
}
