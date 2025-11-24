add_library(trdp-core STATIC
    src/core.cpp
)

set(TRDP_LIB_PATH "" CACHE PATH "Path to the directory containing libtrdp.a")
set(TRDP_INCLUDE_DIR "" CACHE PATH "Path to the TRDP include directory (tau_xml.h, etc.)")

find_library(TRDP_LIBRARY
    NAMES trdp
    PATHS ${TRDP_LIB_PATH}
    NO_DEFAULT_PATH
)

if(NOT TRDP_LIBRARY)
    message(FATAL_ERROR "TRDP library not found. Please set TRDP_LIB_PATH to the directory containing libtrdp.a")
endif()

if(NOT EXISTS ${TRDP_INCLUDE_DIR})
    message(FATAL_ERROR "TRDP include directory not found. Please set TRDP_INCLUDE_DIR to the directory containing TRDP headers (e.g., tau_xml.h)")
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
