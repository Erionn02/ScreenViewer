#pragma once

#include "IOController.hpp"
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>


class X11IOControllerException : public IOControllerException {
public:
    using IOControllerException::IOControllerException;
};


class X11IOController : public IOController {
public:
    X11IOController();
    void handleKeyboardEvent(KeyboardEventData event_data) override;
    void handleMouseEvent(MouseEventData event_data) override;
    cv::Mat captureScreenshot() override;

private:
    std::unique_ptr<Display, decltype(&XCloseDisplay)> createDisplay();
    Window getWindow();
    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> getScreensInfo();


    int screen_count{0};
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display;
    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> screens;
    Window root;
};


