add_executable(trdp-backend)

target_sources(trdp-backend
    PRIVATE
        src/main.cpp
        src/controllers/TrdpController.cc
        src/json_utils.cpp
)

target_include_directories(trdp-backend
    PRIVATE
        ${PROJECT_SOURCE_DIR}/backend/include
        ${PROJECT_SOURCE_DIR}/trdp-core/include
)

target_link_libraries(trdp-backend
    PRIVATE
        trdp-core
        ${DROGON_TARGET}
)

target_compile_features(trdp-backend PRIVATE cxx_std_17)
