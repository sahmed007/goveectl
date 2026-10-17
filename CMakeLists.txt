
cmake_minimum_required(VERSION 3.21)

project(goveectl VERSION 0.1.0 LANGUAGES CXX)

# C++ configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clangd / Zed
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
include(FetchContent)

# Use the system libcurl instead of building it
set(CPR_USE_SYSTEM_CURL ON CACHE BOOL "" FORCE)
set(CPR_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(UUID_SYSTEM_GENERATOR ON CACHE BOOL "" FORCE)
set(UUID_USING_CXX20_SPAN ON CACHE BOOL "" FORCE)
set(UUID_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 3139b2682f0b1d6af8c7ae48cf400f9d9988deec
)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)

FetchContent_Declare(
    stduuid
    GIT_REPOSITORY https://github.com/mariusbancila/stduuid.git
    GIT_TAG daedc9a0029efec422d252e4152a776285a68006
)

FetchContent_MakeAvailable(json)
FetchContent_MakeAvailable(cpr)
FetchContent_MakeAvailable(stduuid)

# Executable
add_executable(goveectl
    src/main.cpp
    src/govee.cpp
    src/cli.cpp
)

# Third-party headers (CLI11)
target_include_directories(goveectl SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/external
)

target_include_directories(goveectl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link dependencies
target_link_libraries(goveectl PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    stduuid
)

# Compiler warnings
if(MSVC)
  target_compile_options(goveectl PRIVATE
        /W4
        /WX
    )
else()
  target_compile_options(goveectl PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
endif()
