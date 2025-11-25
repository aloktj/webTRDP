cmake_minimum_required(VERSION 3.16)
project(webTRDP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Third-party dependencies
set(_drogon_cmake "${CMAKE_CURRENT_SOURCE_DIR}/third_party/drogon/CMakeLists.txt")
if(NOT EXISTS "${_drogon_cmake}")
    message(FATAL_ERROR "Drogon submodule is missing. Run 'git submodule update --init --recursive third_party/drogon' or vendor Drogon manually before configuring.")
endif()
add_subdirectory(third_party/drogon)

# Project targets
add_subdirectory(trdp-core)
add_subdirectory(backend)

# Enable testing hooks for future phases
include(CTest)
