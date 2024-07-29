#pragma once

#include "ScreenViewerBaseException.hpp"
#include "ClientSocket.hpp"

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
    ScreenViewerStreamer(ClientSocket socket);

    void run();
private:
    void captureScreenshot();
    void handleMouseEvent(MouseEventData event_data);
    void handleKeyboardEvent(KeyboardEventData event_data);
    void drawCursor(cv::Mat &img);
    std::unique_ptr<Display, decltype(&XCloseDisplay)> createDisplay();
    Window getWindow();
    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> getScreensInfo();
    void runInputPollerThread();
    void handleInput(const OwnedMessage &message);



    ClientSocket socket;
    std::uint32_t fps;
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display;
    int screen_count{0};
    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> screens;
    Window root;
    std::vector<uchar> frame_buffer{};
    tbb::concurrent_queue<OwnedMessage> messages{};
    std::jthread message_poller_thread;
    void handleIOEvents();
};


