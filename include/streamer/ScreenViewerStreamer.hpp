#pragma once

#include "ScreenViewerBaseException.hpp"
#include "ClientSocket.hpp"
#include "IOController.hpp"
#include "VideoEncoder.hpp"

#include <tbb/concurrent_queue.h>
#include <opencv2/opencv.hpp>
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
    void scheduleAsyncPollIOEvents();
    void scheduleAsyncSendScreenshots();
    void handleInput(const OwnedMessage &message);
    void handleIOEvents();
    VideoEncoder createVideoEncoder();


    std::shared_ptr<ClientSocket> socket;
    std::unique_ptr<IOController> io_controller;
    VideoEncoder encoder;
    tbb::concurrent_queue<OwnedMessage> messages{};
};


