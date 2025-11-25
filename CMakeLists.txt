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
set(TRDP_SIMULATOR_NAME "trdp-simulator")
set(TRDP_SIMULATOR_XML_SOURCE_DIR "${PROJECT_SOURCE_DIR}/backend/xml")
set(TRDP_SIMULATOR_DEFAULT_XML_DIR "${TRDP_SIMULATOR_XML_SOURCE_DIR}/defaults")
set(TRDP_SIMULATOR_FRONTEND_BUILD_DIR "${PROJECT_SOURCE_DIR}/frontend/dist")

install(DIRECTORY DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/${TRDP_SIMULATOR_NAME}")
install(DIRECTORY DESTINATION "${CMAKE_INSTALL_LOCALSTATEDIR}/lib/${TRDP_SIMULATOR_NAME}")

install(DIRECTORY DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${TRDP_SIMULATOR_NAME}/xml/defaults")

if(EXISTS "${TRDP_SIMULATOR_XML_SOURCE_DIR}")
    install(
        DIRECTORY ${TRDP_SIMULATOR_XML_SOURCE_DIR}/
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${TRDP_SIMULATOR_NAME}/xml"
        FILES_MATCHING
        PATTERN "*.xml"
    )
endif()

if(EXISTS "${TRDP_SIMULATOR_DEFAULT_XML_DIR}")
    install(
        DIRECTORY ${TRDP_SIMULATOR_DEFAULT_XML_DIR}/
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${TRDP_SIMULATOR_NAME}/xml/defaults"
        FILES_MATCHING
        PATTERN "*.xml"
    )
endif()

if(EXISTS "${TRDP_SIMULATOR_FRONTEND_BUILD_DIR}")
    install(
        DIRECTORY ${TRDP_SIMULATOR_FRONTEND_BUILD_DIR}/
        DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/${TRDP_SIMULATOR_NAME}/frontend"
    )
endif()

set(CPACK_PACKAGE_NAME "${TRDP_SIMULATOR_NAME}")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "TRDP Simulator Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "TRDP Simulator backend and supporting components")
set(CPACK_PACKAGE_CONTACT "Alok T J <aloktj97@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alok T J <aloktj97@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_SET_DESTDIR ON)

if(NOT DEFINED CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
    find_program(DPKG_EXECUTABLE dpkg)
    if(DPKG_EXECUTABLE)
        execute_process(
            COMMAND ${DPKG_EXECUTABLE} --print-architecture
            OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif()
    if(NOT CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "all")
    endif()
endif()

include(CPack)
set(CPACK_GENERATOR "DEB")
