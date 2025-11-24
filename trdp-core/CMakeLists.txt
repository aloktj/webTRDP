add_library(trdp-core STATIC
    src/core.cpp
)

target_include_directories(trdp-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(trdp-core PUBLIC cxx_std_17)
