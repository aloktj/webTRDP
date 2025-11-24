add_executable(trdp-backend)

target_sources(trdp-backend
    PRIVATE
        src/main.cpp
)

target_include_directories(trdp-backend
    PRIVATE
        ${PROJECT_SOURCE_DIR}/trdp-core/include
)

target_link_libraries(trdp-backend
    PRIVATE
        trdp-core
        Drogon::Drogon
)

target_compile_features(trdp-backend PRIVATE cxx_std_17)
