cmake_minimum_required(VERSION 3.21)

project(goveectl VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(goveectl src/main.cpp)

target_compile_features(goveectl PRIVATE cxx_std_20)

target_include_directories(goveectl SYSTEM PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/external
)

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
