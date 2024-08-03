include(CMakePrintHelpers)

add_lib(screen-viewer-lib
        SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/SocketBase.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/MessageHeader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ClientSocket.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AuthenticatedSession.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ScreenViewerClient.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/streamer/ScreenViewerStreamer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/streamer/X11IOController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/streamer/WaylandIOController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/KeysMapping.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/MouseConfig.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/UsersManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ServerSessionsManager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ProxySession.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/VideoEncoder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/VideoDecoder.cpp
        )

