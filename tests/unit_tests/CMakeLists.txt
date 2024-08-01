set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -w -O0 -g3 -no-pie")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_DEBUG} -w -O0 -g3 -no-pie")

package_add_test(unit-tests
        SOURCES
        MessageHeaderTests.cpp
        TCPBridgeTests.cpp
        ServerSessionsManagerTests.cpp
        VideoEncoderDecoderTests.cpp
        DEPENDS screen-viewer-lib
        )

target_include_directories(unit-tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_compile_definitions(unit-tests PRIVATE TEST_DIR="${CMAKE_SOURCE_DIR}/tests/test_assets")

