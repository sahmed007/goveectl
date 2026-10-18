
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
    src/commands.cpp
    src/display.cpp
    src/govee.cpp
    src/govee_parsing.cpp
    src/device_selection.cpp
    src/device_cache.cpp
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

# Tests have their own main() and compile only offline application code.
include(CTest)
if(BUILD_TESTING)
    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
        GIT_SHALLOW TRUE
    )
    # doctest 2.4.11 declares an old minimum; CMake 4 needs this policy floor.
    # Function scope keeps the compatibility setting local to this dependency.
    function(make_doctest_available)
        set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
        FetchContent_MakeAvailable(doctest)
    endfunction()
    make_doctest_available()

    add_executable(goveectl_tests
        tests/main.cpp
        tests/cli_test.cpp
        tests/govee_parsing_test.cpp
        tests/device_selection_test.cpp
        tests/device_cache_test.cpp
        tests/display_test.cpp
        src/govee_parsing.cpp
        src/device_selection.cpp
        src/device_cache.cpp
        src/display.cpp
        src/cli.cpp
    )
    target_include_directories(goveectl_tests PRIVATE include)
    target_include_directories(goveectl_tests SYSTEM PRIVATE external)
    # Keep third-party implementation warnings out of clangd's -Werror checks.
    target_include_directories(goveectl_tests SYSTEM PRIVATE
        $<TARGET_PROPERTY:doctest::doctest,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_link_libraries(goveectl_tests PRIVATE doctest::doctest nlohmann_json::nlohmann_json)
    if(MSVC)
        target_compile_options(goveectl_tests PRIVATE /W4 /WX)
    else()
        target_compile_options(goveectl_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME goveectl_cli COMMAND goveectl_tests)
endif()
