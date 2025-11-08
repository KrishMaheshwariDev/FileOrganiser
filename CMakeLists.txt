cmake_minimum_required(VERSION 3.20)
project(FolderSort)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# compilers path
set(CMAKE_CXX_COMPILER "C:/mingw64/bin/g++.exe")
set(CMAKE_C_COMPILER "C:/mingw64/bin/gcc.exe")

# Define project source directories
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(STATES_DIR "${SRC_DIR}/States")
set(MANAGERS_DIR "${SRC_DIR}/Managers")

# Add all source files
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${STATES_DIR}/EntryState.cpp
    ${MANAGERS_DIR}/SearchManager.cpp
    ${MANAGERS_DIR}/TagManager.cpp
)

# Include header directories
include_directories(${CMAKE_SOURCE_DIR}/include)

include_directories(${SRC_DIR}/States)
include_directories(${SRC_DIR}/Managers)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create the executable
add_executable(FolderSort ${SOURCES})

# Link Windows-specific library for PathIsDirectoryA
if (WIN32)
    target_link_libraries(FolderSort shlwapi)
endif()
