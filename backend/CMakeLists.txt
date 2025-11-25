add_executable(trdp-backend)

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
        TRDP_DEFAULT_XML_PATH="${PROJECT_SOURCE_DIR}/third_party/TCNopen/trdp/test/xml/example.xml"
        TRDP_DEFAULT_HOST_NAME="localhost"
        TRDP_DEFAULT_CONFIG_DIR="/etc/webtrdp/configs"
)

target_link_libraries(trdp-backend
    PRIVATE
        trdp-core
        ${DROGON_TARGET}
)

target_compile_features(trdp-backend PRIVATE cxx_std_17)
