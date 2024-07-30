#include "streamer/X11IOController.hpp"
#include "MouseConfig.hpp"


#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XTest.h>
#include <spdlog/spdlog.h>
#include <opencv2/imgcodecs.hpp>


#include <iostream>


X11IOController::X11IOController(): display(createDisplay()), screens(getScreensInfo()), root(getWindow()) {}


std::unique_ptr<Display, decltype(&XCloseDisplay)> X11IOController::createDisplay() {
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display_local{XOpenDisplay(NULL), XCloseDisplay};
    if (!display_local) {
        throw X11IOControllerException("Cannot open display");
    }
    return display_local;
}

Window X11IOController::getWindow() {
    return DefaultRootWindow(display.get());
}

std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> X11IOController::getScreensInfo() {
    if (!XineramaIsActive(display.get())) {
        throw X11IOControllerException("Xinerama is not active");
    }

    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> screens_local{
            XineramaQueryScreens(display.get(), &screen_count), XFree};
    if (!screens_local) {
        throw X11IOControllerException("Failed to query Xinerama screens");
    }
    if (screen_count < 1) {
        throw X11IOControllerException(fmt::format("Screen count ({})<1", screen_count));
    }
    return screens_local;
}

void X11IOController::handleKeyboardEvent(KeyboardEventData event_data) {
    KeyCode keycode = XKeysymToKeycode(display.get(), event_data.key);
    spdlog::info("Got keycode: {}", keycode);
    if (keycode == 0) {
        std::cerr << "Invalid key symbol" << std::endl;
        return;
    }

    XTestFakeKeyEvent(display.get(), keycode, event_data.down, CurrentTime);
    XFlush(display.get());
}

void X11IOController::handleMouseEvent(MouseEventData event_data) {
    spdlog::info("Mouse event, mask: {}, x: {}, y: {}", event_data.button_mask, event_data.x, event_data.y);


    if (event_data.button_mask == Mouse::MOVE) {
        event_data.x += screens->x_org;
        event_data.y += screens->y_org;
        XTestFakeMotionEvent(display.get(), -1, event_data.x, event_data.y, CurrentTime);
        XFlush(display.get());
        return;
    }

    int button = Mouse::extractButtonID(event_data.button_mask);
    if (Mouse::isScrollMoved(event_data.button_mask)) {
        XTestFakeButtonEvent(display.get(), static_cast<unsigned int>(button), True, CurrentTime);
        XTestFakeButtonEvent(display.get(), static_cast<unsigned int>(button), False, CurrentTime);
    } else if (button != 0) {
        bool is_clicked = Mouse::isClicked(event_data.button_mask);
        XTestFakeButtonEvent(display.get(), static_cast<unsigned int>(button), is_clicked, CurrentTime);
    }
    XFlush(display.get());
}

cv::Mat X11IOController::captureScreenshot() {
    struct DestroyXImage {
        void operator()(XImage *image) {
            XDestroyImage(image);
        }
    };
    std::unique_ptr<XImage, DestroyXImage> image{
            XGetImage(display.get(), RootWindow(display.get(), DefaultScreen(display.get())),
                      screens->x_org, screens->y_org, static_cast<unsigned int>(screens->width),
                      static_cast<unsigned int>(screens->height), AllPlanes, ZPixmap), DestroyXImage{}};
    if (!image) {
        throw X11IOControllerException("Failed to capture screen");
    }

    cv::Mat img = cv::Mat(screens->height, screens->width, CV_8UC4, image->data);
    std::unique_ptr<XFixesCursorImage, decltype(&XFree)> cursor_image{XFixesGetCursorImage(display.get()), XFree};
    if (cursor_image) {
        cv::Mat cursor_mat{cursor_image->height, cursor_image->width, CV_8UC4, cursor_image->pixels};
        cv::Point offset{cursor_image->x - cursor_image->xhot, cursor_image->y - cursor_image->yhot};
        drawCursor(img, cursor_mat, offset);
    }

    return img;
}
