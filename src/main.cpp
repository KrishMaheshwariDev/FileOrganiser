#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include "../include/json/json.hpp"
#include <filesystem>

#include "Managers/SearchManager.h"
#include "Managers/TagManager.h"
#include "Managers/FileManager.h"

namespace fs = std::filesystem;

// Forward declarations
void DrawTagPanel(TagManager &tagManager, std::string &selectedTag, std::string &destinationEdit);
void DrawFilePanel(SearchManager &searchManager, TagManager &tagManager, FileManager &fileManager, const std::string &selectedTag);
void DrawTopMenu(SearchManager &searchManager, std::string &currentDir);

// -------------------------------------------------------------

int main()
{
    if (!glfwInit())
        return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(1280, 720, "FolderSort", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Backends
    SearchManager searchManager(SearchMode::TOP_LEVEL);
    TagManager tagManager(searchManager);
    FileManager fileManager(tagManager, searchManager);

    std::string currentDir = fs::current_path().string();
    std::string selectedTag;
    std::string destinationEdit;

    // Preload current directory
    searchManager.LoadMetaData(currentDir, SearchMode::TOP_LEVEL);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("FolderSort Tool", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse);
        DrawTopMenu(searchManager, currentDir);
        ImGui::Separator();
        ImGui::Columns(2);
        DrawTagPanel(tagManager, selectedTag, destinationEdit);
        ImGui::NextColumn();
        DrawFilePanel(searchManager, tagManager, fileManager, selectedTag);
        ImGui::Columns(1);
        ImGui::End();

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// -------------------------------------------------------------
// Menu: Load Directory
void DrawTopMenu(SearchManager &searchManager, std::string &currentDir)
{
    if (ImGui::Button("Load Directory"))
    {
        char pathBuf[512] = {};
        ImGui::OpenPopup("LoadDirPopup");
    }

    if (ImGui::BeginPopup("LoadDirPopup"))
    {
        static char dirPath[512] = {};
        ImGui::InputText("Directory Path", dirPath, IM_ARRAYSIZE(dirPath));
        if (ImGui::Button("Load"))
        {
            if (std::filesystem::exists(dirPath))
            {
                currentDir = dirPath;
                searchManager.LoadMetaData(currentDir, SearchMode::TOP_LEVEL);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::SameLine();
    ImGui::Text("Current Directory: %s", currentDir.c_str());
}

// -------------------------------------------------------------
// Tag panel with editable destination
void DrawTagPanel(TagManager &tagManager, std::string &selectedTag, std::string &destinationEdit)
{
    ImGui::BeginChild("TagPanel", ImVec2(300, 0), true);
    ImGui::Text("Tags:");
    ImGui::Separator();

    const auto &tagMap = tagManager.GetTagMap();
    for (const auto &[tag, _] : tagMap)
    {
        bool isSelected = (selectedTag == tag);
        if (ImGui::Selectable(tag.c_str(), isSelected))
        {
            selectedTag = tag;
            // Load current destination from JSON or memory
            destinationEdit.clear();
            std::ifstream ifs("tags.json");
            nlohmann::json j;
            if (ifs.is_open())
            {
                ifs >> j;
                ifs.close();
                if (j.contains("tags") && j["tags"].contains(tag))
                    destinationEdit = j["tags"][tag]["destination"].get<std::string>();
            }
        }
    }

    static char newTagName[128] = {};
    if (ImGui::InputText("New Tag", newTagName, IM_ARRAYSIZE(newTagName), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (strlen(newTagName) > 0)
        {
            tagManager.CreateTag(newTagName);
            newTagName[0] = 0;
        }
    }

    if (!selectedTag.empty())
    {
        ImGui::Separator();
        ImGui::Text("Destination for '%s':", selectedTag.c_str());
        static char destBuf[512];
        strcpy(destBuf, destinationEdit.c_str());
        if (ImGui::InputText("##dest", destBuf, IM_ARRAYSIZE(destBuf)))
            destinationEdit = destBuf;

        if (ImGui::Button("Update Destination"))
        {
            if (!destinationEdit.empty())
                tagManager.SetDestination(selectedTag, destinationEdit);
        }

        if (ImGui::Button("Delete Tag"))
        {
            tagManager.DeleteTag(selectedTag);
            selectedTag.clear();
        }
    }

    ImGui::EndChild();
}

// -------------------------------------------------------------
void DrawFilePanel(SearchManager &searchManager, TagManager &tagManager, FileManager &fileManager, const std::string &selectedTag)
{
    ImGui::BeginChild("FilePanel", ImVec2(0, 0), true);

    const auto &files = searchManager.GetAllFiles();

    ImGui::Text("Files in Current Directory");
    ImGui::Separator();

    ImGui::Columns(3);
    ImGui::Text("ID");
    ImGui::NextColumn();
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("Path");
    ImGui::NextColumn();
    ImGui::Separator();

    for (const auto &file : files)
    {
        ImGui::Text("%d", file.fileID);
        ImGui::NextColumn();
        ImGui::Text("%s", file.name.c_str());
        ImGui::NextColumn();
        ImGui::TextWrapped("%s", file.path.string().c_str());
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::Separator();

    if (ImGui::Button("Assign Selected Tag to All Files") && !selectedTag.empty())
    {
        for (const auto &file : files)
            tagManager.AssignTag(file.path, selectedTag);
    }

    if (ImGui::Button("Move All Tagged Files"))
        fileManager.MoveAllTaggedFiles();

    if (ImGui::Button("Move Selected Tag Files") && !selectedTag.empty())
        fileManager.MoveFilesByTag(selectedTag);

    ImGui::EndChild();
}
