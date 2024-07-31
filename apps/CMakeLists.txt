include(CMakePrintHelpers)

add_app(ScreenViewerServer
        ScreenViewerServer.cpp
        )

add_app(ScreenViewerClient
        ScreenViewerClient.cpp
        )

add_app(ScreenViewerStreamer
        ScreenViewerStreamer.cpp
        )

add_app(add_user
        add_user.cpp
        )

add_app(test_encode_decode
        test_encode_decode.cpp
)


target_compile_definitions(ScreenViewerServer PRIVATE TEST_CERTS_DIR="${CMAKE_SOURCE_DIR}/tests/test_certs")