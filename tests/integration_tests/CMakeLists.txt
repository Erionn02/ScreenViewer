set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -w -O0 -g3 -no-pie")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG} -w -O0 -g3 -no-pie")


package_add_test(integration-tests
        SOURCES
        SocketTests.cpp
        UsersManagerTests.cpp
        SSLBridgeTest.cpp
        DEPENDS screen-viewer-lib
        )

target_compile_definitions(integration-tests PRIVATE CERTS_DIR="${CMAKE_SOURCE_DIR}/tests/test_certs")
target_include_directories(integration-tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
