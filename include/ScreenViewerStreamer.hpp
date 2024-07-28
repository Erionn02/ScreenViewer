#pragma once

#include "ScreenViewerBaseException.hpp"
#include "ClientSocket.hpp"

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
    void scheduleAsyncInputHandling();
    void handleInput(BorrowedMessage message);



    ClientSocket socket;
    std::uint32_t fps;
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display;
    int screen_count{0};
    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> screens;
    Window root;
    std::vector<uchar> frame_buffer{};
    std::jthread input_handler_thread;
};


