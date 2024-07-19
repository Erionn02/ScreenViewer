include(CMakePrintHelpers)

add_lib(screen-viewer-lib
        SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/SocketBase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/MessageHeader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ClientSocket.cpp
        )

