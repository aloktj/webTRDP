cmake_minimum_required(VERSION 3.16)
project(webTRDP VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Install prefix" FORCE)
endif()

set(CMAKE_INSTALL_SYSCONFDIR "/etc" CACHE PATH "System configuration directory" FORCE)
set(CMAKE_INSTALL_LOCALSTATEDIR "/var" CACHE PATH "Local state directory" FORCE)

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

install(FILES README.md LICENSE DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Enable testing hooks for future phases
include(CTest)

# Packaging configuration
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "webtrdp")
set(CPACK_PACKAGE_VENDOR "webTRDP Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "TRDP Simulator backend and supporting components")
set(CPACK_PACKAGE_CONTACT "webTRDP Maintainers <maintainers@example.com>")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "webTRDP Maintainers")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

include(CPack)
