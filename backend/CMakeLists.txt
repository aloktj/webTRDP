add_executable(trdp-backend)

set(WEBTRDP_DEFAULT_XML_PATH "${CMAKE_INSTALL_SYSCONFDIR}/webtrdp/configs/trdp_config.xml" CACHE STRING "Default TRDP XML configuration path")
set(WEBTRDP_DEFAULT_HOST_NAME "localhost" CACHE STRING "Default TRDP host name")
set(WEBTRDP_DEFAULT_CONFIG_DIR "${CMAKE_INSTALL_SYSCONFDIR}/webtrdp/configs" CACHE STRING "Default TRDP configuration directory")
set(WEBTRDP_STATE_DIR "${CMAKE_INSTALL_LOCALSTATEDIR}/lib/webtrdp" CACHE PATH "State directory for webTRDP runtime files")

option(WEBTRDP_INSTALL_SYSTEMD_UNIT "Install the systemd unit for the TRDP backend" ON)

if(NOT DEFINED CMAKE_INSTALL_SYSTEMD_UNITDIR)
    set(CMAKE_INSTALL_SYSTEMD_UNITDIR "lib/systemd/system" CACHE PATH "Systemd unit directory")
endif()

target_sources(trdp-backend
    PRIVATE
        src/main.cpp
        src/controllers/TrdpController.cc
        src/config_paths.cpp
        src/json_utils.cpp
)

target_include_directories(trdp-backend
    PRIVATE
        ${PROJECT_SOURCE_DIR}/backend/include
        ${PROJECT_SOURCE_DIR}/trdp-core/include
)

target_compile_definitions(trdp-backend
    PRIVATE
        TRDP_DEFAULT_XML_PATH="${WEBTRDP_DEFAULT_XML_PATH}"
        TRDP_DEFAULT_HOST_NAME="${WEBTRDP_DEFAULT_HOST_NAME}"
        TRDP_DEFAULT_CONFIG_DIR="${WEBTRDP_DEFAULT_CONFIG_DIR}"
)

target_link_libraries(trdp-backend
    PRIVATE
        trdp-core
        ${DROGON_TARGET}
)

target_compile_features(trdp-backend PRIVATE cxx_std_17)

install(TARGETS trdp-backend RUNTIME DESTINATION bin)

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/webtrdp)
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/webtrdp/configs)
install(DIRECTORY DESTINATION ${WEBTRDP_STATE_DIR})

configure_file(${PROJECT_SOURCE_DIR}/cmake/webtrdp.default ${CMAKE_CURRENT_BINARY_DIR}/webtrdp.default @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/webtrdp.default DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/default RENAME webtrdp)

if(WEBTRDP_INSTALL_SYSTEMD_UNIT)
    configure_file(${PROJECT_SOURCE_DIR}/cmake/webtrdp.service.in ${CMAKE_CURRENT_BINARY_DIR}/webtrdp.service @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/webtrdp.service DESTINATION ${CMAKE_INSTALL_SYSTEMD_UNITDIR})
endif()
