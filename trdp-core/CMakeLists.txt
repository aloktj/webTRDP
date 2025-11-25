add_library(trdp-core STATIC
    src/trdp_engine.cpp
    src/trdp_config_loader.cpp
)

set(TRDP_USE_SUBMODULE ON CACHE BOOL "Build TRDP from the bundled TCNopen submodule if available")
set(TRDP_LIB_PATH "" CACHE PATH "Path to the directory containing libtrdp.a (when not using the submodule)")
set(TRDP_INCLUDE_DIR "" CACHE PATH "Path to the TRDP include directory (tau_xml.h, etc.) when provided manually")

set(_tcnopen_dir "${PROJECT_SOURCE_DIR}/third_party/TCNopen/trdp")
if(TRDP_USE_SUBMODULE AND EXISTS "${_tcnopen_dir}/CMakeLists.txt")
    add_subdirectory(${_tcnopen_dir} ${CMAKE_BINARY_DIR}/tcnopen)
    set(TRDP_LIBRARY TRDP::trdp)
else()
    find_library(TRDP_LIBRARY
        NAMES trdp
        PATHS ${TRDP_LIB_PATH}
        NO_DEFAULT_PATH
    )

    if(NOT TRDP_LIBRARY)
        message(FATAL_ERROR "TRDP library not found. Please set TRDP_LIB_PATH to the directory containing libtrdp.a or enable TRDP_USE_SUBMODULE to build from third_party/TCNopen")
    endif()

    if(NOT EXISTS ${TRDP_INCLUDE_DIR})
        message(FATAL_ERROR "TRDP include directory not found. Please set TRDP_INCLUDE_DIR to the directory containing TRDP headers (e.g., tau_xml.h)")
    endif()
endif()

target_include_directories(trdp-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${TRDP_INCLUDE_DIR}
)

target_link_libraries(trdp-core
    PUBLIC
        ${TRDP_LIBRARY}
)

target_compile_features(trdp-core PUBLIC cxx_std_17)

add_executable(trdp_config_dump tests/trdp_config_dump.cpp)
target_link_libraries(trdp_config_dump PRIVATE trdp-core)
