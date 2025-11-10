cmake_minimum_required(VERSION 3.15)
project(FolderSort LANGUAGES CXX)

# -------------------------------
#  Compiler configuration
# -------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional: show all warnings
if (MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Compiler warning control ---
if (MSVC)
    # /W0 disables all warnings
    add_compile_options(/W0)
else()
    # -w disables all warnings for GCC/Clang (MinGW included)
    add_compile_options(-w)
endif()


# -------------------------------
#  OpenGL + GLFW + ImGui
# -------------------------------
find_package(OpenGL REQUIRED)

# If you have GLFW source under include/glfw, build it manually:
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(include/glfw)

# -------------------------------
#  ImGui setup
# -------------------------------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(imgui PUBLIC OpenGL::GL glfw)

# -------------------------------
#  Your application
# -------------------------------
file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(FolderSort ${SRC_FILES})

target_include_directories(FolderSort PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(FolderSort PRIVATE
    imgui
    glfw
    OpenGL::GL
    shlwapi       # ✅ required for PathIsDirectoryA
)


# -------------------------------
#  Output directory
# -------------------------------
set_target_properties(FolderSort PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
