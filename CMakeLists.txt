cmake_minimum_required(VERSION 3.16)
project(webTRDP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Third-party dependencies
set(_drogon_cmake "${CMAKE_CURRENT_SOURCE_DIR}/third_party/drogon/CMakeLists.txt")
if(EXISTS "${_drogon_cmake}")
    add_subdirectory(third_party/drogon)
else()
    find_package(Drogon CONFIG)

    if(NOT Drogon_FOUND)
        message(FATAL_ERROR "Drogon dependency not available. Either initialize the 'third_party/drogon' submodule (git submodule update --init --recursive third_party/drogon) or install Drogon so CMake can find it.")
    endif()
endif()

# Normalize Drogon target name regardless of how it is provided
if(TARGET drogon::drogon)
    set(DROGON_TARGET drogon::drogon)
elseif(TARGET Drogon::Drogon)
    set(DROGON_TARGET Drogon::Drogon)
elseif(TARGET drogon)
    add_library(drogon::drogon ALIAS drogon)
    set(DROGON_TARGET drogon::drogon)
else()
    message(FATAL_ERROR "Drogon target not found even though the package/submodule was configured.")
endif()

# Project targets
add_subdirectory(trdp-core)
add_subdirectory(backend)

# Enable testing hooks for future phases
include(CTest)
