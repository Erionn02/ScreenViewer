include(CMakePrintHelpers)

add_app(ScreenViewerServer
        ScreenViewerServer.cpp
        )

add_app(ScreenViewerClient
        ScreenViewerClient.cpp
        )

add_app(vnc_client
        vnc_client.cpp
        )

add_app(vnc_server
        vnc_server.cpp
        )

add_app(add_user
        add_user.cpp
        )



target_compile_definitions(ScreenViewerServer PRIVATE TEST_CERTS_DIR="${CMAKE_SOURCE_DIR}/tests/test_certs")