#include <string>
#include <iostream>

enum class Mode
{
    RECURSIVE_SEARCH,
    TOP_LEVEL_SEARCH
};

class EntryState
{
private:
    Mode mode;
    std::string directoryPath;

public:
    // Methods to Validate Path and get Mode
    bool ValidatePath(const std::string &directoryPath);
    bool AskMode();

    // void StartSearch(const std::string& directoryPath, Mode mode);  // Commented for now until SearchManager is not defined
    void Run();
};