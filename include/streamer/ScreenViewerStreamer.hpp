#pragma once

#include "ScreenViewerBaseException.hpp"
#include "ClientSocket.hpp"
#include "IOController.hpp"

#include <tbb/concurrent_queue.h>
#include <opencv2/opencv.hpp>
#include <rfb/rfb.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

#include <memory>


class VNCServerException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


class ScreenViewerStreamer {
public:
    ScreenViewerStreamer(std::shared_ptr<ClientSocket> socket, std::unique_ptr<IOController> io_controller);

    void run();
private:
    void scheduleAsyncReadMessage();
    void handleInput(const OwnedMessage &message);
    void handleIOEvents();


    std::shared_ptr<ClientSocket> socket;
    std::unique_ptr<IOController> io_controller;

    std::vector<uchar> frame_buffer{};
    tbb::concurrent_queue<OwnedMessage> messages{};
};


