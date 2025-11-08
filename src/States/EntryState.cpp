#include <shlwapi.h>
#include <string>
#include "EntryState.h"

void EntryState::Run()
{
    std::string Path;
    do
    {
        std::cout << "Enter the Directory path: ";
        std::getline(std::cin, Path);
        std::cout << "\n";
    } while (!ValidatePath(Path));

    while (!AskMode())
    {
    }

    std::cout << "Starting to Search the Folder" << "\n";

    // StartSearch(this->directoryPath, this->mode);  // when the SearchManager is defined, uncomment this line
}

bool EntryState::ValidatePath(const std::string &directoryPath)
{
    if (!PathIsDirectoryA(directoryPath.c_str()))
    {
        std::cout << "Entered directory path is invalid try again." << "\n";
        return false;
    }
    else
    {
        this->directoryPath = directoryPath;
        return true;
    }
}

bool EntryState::AskMode()
{
    std::cout << "Enter the Search Mode (RECURSIVE_SEARCH: 0, TOP_LEVEL_SEARCH: 1) : ";
    int input;
    std::cin >> input;
    std::cout << "\n";

    switch (input)
    {
    case 0:
        this->mode = Mode::RECURSIVE_SEARCH;
        return true;

    case 1:
        this->mode = Mode::TOP_LEVEL_SEARCH;
        return true;

    default:
        std::cout << "Invalid Search Mode, try again" << "\n";
        return false;
    }
}