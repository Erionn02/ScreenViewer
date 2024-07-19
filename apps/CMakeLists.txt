include(CMakePrintHelpers)

add_app(ScreenViewerServer
        ScreenViewerServer.cpp
        )

add_app(ScreenViewerClient
        ScreenViewerClient.cpp
        )

target_compile_definitions(ScreenViewerServer PRIVATE TEST_CERTS_DIR="${CMAKE_SOURCE_DIR}/tests/test_certs")