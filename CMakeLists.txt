cmake_minimum_required(VERSION 3.16)

project(webTRDP LANGUAGES C)

# Keep the TRDP build lean while enabling the SOA helpers we reference.
set(TRDP_SOA_SUPPORT ON CACHE BOOL "Enable TRDP SOA helpers" FORCE)
set(TRDP_BUILD_EXAMPLES OFF CACHE BOOL "Disable upstream TRDP example builds" FORCE)
set(TRDP_BUILD_TSN_EXAMPLES OFF CACHE BOOL "Disable upstream TRDP TSN example builds" FORCE)
set(TRDP_BUILD_TESTS OFF CACHE BOOL "Disable upstream TRDP test builds" FORCE)
set(TRDP_BUILD_XML_TOOLS OFF CACHE BOOL "Disable upstream TRDP XML helper builds" FORCE)
set(TRDP_BUILD_LOCAL_TESTS OFF CACHE BOOL "Disable upstream TRDP local loopback tests" FORCE)
set(TRDP_BUILD_MARSHALLING_TOOLS OFF CACHE BOOL "Disable upstream TRDP marshalling demos" FORCE)

add_subdirectory(third_party/TCNopen/trdp)

add_executable(trdp_api_reference
    src/trdp_api_reference.c
)

target_compile_features(trdp_api_reference PRIVATE c_std_99)

target_link_libraries(trdp_api_reference PRIVATE TRDP::trdpap)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME trdp_api_reference COMMAND trdp_api_reference)
endif()
