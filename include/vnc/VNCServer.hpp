#pragma once

#include "ScreenViewerBaseException.hpp"

#include <opencv2/opencv.hpp>
#include <rfb/rfb.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>

#include <memory>


class VNCServerException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


class VNCServer {
public:
    VNCServer(std::uint32_t fps);

    void run();
private:
    void captureScreenshot();
    void handlePointerEvent(int buttonMask, int x, int y, rfbClientPtr);
    void handleKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr);
    void drawCursor(cv::Mat &img);
    std::unique_ptr<Display, decltype(&XCloseDisplay)> createDisplay();
    Window getWindow();
    std::unique_ptr<rfbScreenInfo, decltype(&rfbScreenCleanup)> createServer();
    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> getScreensInfo();
    std::unique_ptr<char[]> allocateBuffer();


    std::uint32_t fps;
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display;
    int screen_count{0};
    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> screens;
    Window root;
    std::unique_ptr<char[]> frame_buffer;
    std::unique_ptr<rfbScreenInfo, decltype(&rfbScreenCleanup)> server;
    unsigned int extractButtonID(int buttonMask) const;
};


