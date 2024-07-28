#include "ScreenViewerStreamer.hpp"
#include "MouseConfig.hpp"

#include <spdlog/spdlog.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XTest.h>

#include <chrono>

using namespace std::chrono_literals;


ScreenViewerStreamer::ScreenViewerStreamer(ClientSocket socket) : socket(std::move(socket)),
                                                                  display(createDisplay()), screens(getScreensInfo()),
                                                                  root(getWindow()) {}

std::unique_ptr<Display, decltype(&XCloseDisplay)> ScreenViewerStreamer::createDisplay() {
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display_local{XOpenDisplay(NULL), XCloseDisplay};
    if (!display_local) {
        throw VNCServerException("Cannot open display");
    }
    return display_local;
}

Window ScreenViewerStreamer::getWindow() {
    return DefaultRootWindow(display.get());
}

std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> ScreenViewerStreamer::getScreensInfo() {
    if (!XineramaIsActive(display.get())) {
        throw VNCServerException("Xinerama is not active");
    }

    std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> screens_local{
            XineramaQueryScreens(display.get(), &screen_count), XFree};
    if (!screens_local) {
        throw VNCServerException("Failed to query Xinerama screens");
    }
    if (screen_count < 1) {
        throw VNCServerException(fmt::format("Screen count ({})<1", screen_count));
    }
    return screens_local;
}


void ScreenViewerStreamer::run() {
    spdlog::info("VNC server started");

    scheduleAsyncInputHandling();
    while (true) {
        captureScreenshot();
        socket.send(BorrowedMessage{.type = MessageType::SCREEN_UPDATE,
                .content = {std::bit_cast<char *>(frame_buffer.data()), frame_buffer.size()}});
    }
}

void ScreenViewerStreamer::scheduleAsyncInputHandling() {
    input_handler_thread = std::jthread{[&](const std::stop_token &stop_token) {
        while (!stop_token.stop_requested()) {
            auto message = socket.receiveToBuffer();
            try {
                handleInput(message);
            } catch (const std::exception &e) {
                spdlog::error("Exception while input handling: {}", e.what());
            }

        }
    }};
}

void ScreenViewerStreamer::handleInput(BorrowedMessage message) {
    switch (message.type) {
        case MessageType::KEYBOARD_INPUT: {
            handleKeyboardEvent(convertTo<KeyboardEventData>(message));
            break;
        }
        case MessageType::MOUSE_INPUT: {
            handleMouseEvent(convertTo<MouseEventData>(message));
            break;
        }
        default: {
            spdlog::info("Got unexpected message type: {}", MESSAGE_TYPE_TO_STR.at(message.type));
            break;
        }
    }
}

void ScreenViewerStreamer::captureScreenshot() {
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
        throw VNCServerException("Failed to capture screen");
    }

    cv::Mat img = cv::Mat(screens->height, screens->width, CV_8UC4, image->data);
    drawCursor(img);
    cv::imencode(".png", img, frame_buffer);
}

void ScreenViewerStreamer::drawCursor(cv::Mat &img) {
    std::unique_ptr<XFixesCursorImage, decltype(&XFree)> cursor_image{XFixesGetCursorImage(display.get()), XFree};
    if (!cursor_image) {
        spdlog::warn("Could not get cursor image.");
        return;
    }

    cv::Mat cursor_mat{cursor_image->height, cursor_image->width, CV_8UC4, cursor_image->pixels};

    for (int y = 0; y < cursor_image->height; y++) {
        for (int x = 0; x < cursor_image->width; x++) {
            int dstX = cursor_image->x + x - cursor_image->xhot;
            int dstY = cursor_image->y + y - cursor_image->yhot;

            if (dstX >= 0 && dstX < img.cols && dstY >= 0 && dstY < img.rows) {
                unsigned long pixel = cursor_image->pixels[y * cursor_image->width + x];
                unsigned char alpha = (pixel >> 24) & 0xFF;
                unsigned char red = (pixel >> 16) & 0xFF;
                unsigned char green = (pixel >> 8) & 0xFF;
                unsigned char blue = pixel & 0xFF;

                if (alpha > 0) {
                    cv::Vec4b &bg_pixel = img.at<cv::Vec4b>(dstY, dstX);
                    bg_pixel[0] = static_cast<unsigned char>((blue * alpha + bg_pixel[0] * (255 - alpha)) / 255);
                    bg_pixel[1] = static_cast<unsigned char>((green * alpha + bg_pixel[1] * (255 - alpha)) / 255);
                    bg_pixel[2] = static_cast<unsigned char>((red * alpha + bg_pixel[2] * (255 - alpha)) / 255);
                    bg_pixel[3] = static_cast<unsigned char>(alpha + bg_pixel[3] * (255 - alpha) / 255);
                }
            }
        }
    }
}


void ScreenViewerStreamer::handleMouseEvent(MouseEventData event_data) {
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

void ScreenViewerStreamer::handleKeyboardEvent(KeyboardEventData event_data) {
    KeyCode keycode = XKeysymToKeycode(display.get(), event_data.key);
    spdlog::info("Got keycode: {}", keycode);
    if (keycode == 0) {
        std::cerr << "Invalid key symbol" << std::endl;
        return;
    }

    XTestFakeKeyEvent(display.get(), keycode, event_data.down, CurrentTime);
    XFlush(display.get());
}
