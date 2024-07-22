#include "vnc/VNCServer.hpp"
#include "LambdaToFuncPtr.hpp"

#include <spdlog/spdlog.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/XTest.h>

#include <chrono>

using namespace std::chrono_literals;

constexpr std::size_t PIXEL_WIDTH{4};

VNCServer::VNCServer(std::uint32_t fps) : fps(fps), display(createDisplay()), screens(getScreensInfo()), root(getWindow()),
                                          frame_buffer(allocateBuffer()), server(createServer()) {}

void VNCServer::run() {
    spdlog::info("VNC server started");

    long sleep_time = std::chrono::duration_cast<std::chrono::microseconds>(1s).count() / fps;
    while (rfbIsActive(server.get())) {
        captureScreenshot();
        rfbMarkRectAsModified(server.get(), 0, 0, server->width, server->height);
        rfbProcessEvents(server.get(), sleep_time);
    }
}

std::unique_ptr<Display, decltype(&XCloseDisplay)> VNCServer::createDisplay() {
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display_local{XOpenDisplay(NULL), XCloseDisplay};
    if (!display_local) {
        throw VNCServerException("Cannot open display");
    }
    return display_local;
}

Window VNCServer::getWindow() {
    return DefaultRootWindow(display.get());
}

std::unique_ptr<rfbScreenInfo, decltype(&rfbScreenCleanup)> VNCServer::createServer() {
    std::string program_name{"VNCServer"};
    int argc{1};
    char *argv[]{program_name.data()};
    std::unique_ptr<rfbScreenInfo, decltype(&rfbScreenCleanup)> server_local{
            rfbGetScreen(&argc, argv, screens->width, screens->height, 8, 3, 4), rfbScreenCleanup};
    if (!server_local) {
        throw VNCServerException("Failed to initialize VNC server");
    }

    server_local->frameBuffer = frame_buffer.get();
    server_local->alwaysShared = TRUE;
    server_local->ptrAddEvent = lambdaToPointer(
            [this](int buttonMask, int x, int y, rfbClientPtr ptr) { handlePointerEvent(buttonMask, x, y, ptr); });
    server_local->kbdAddEvent = lambdaToPointer(
            [this](rfbBool down, rfbKeySym key, rfbClientPtr ptr) { handleKeyEvent(down, key, ptr); });

    rfbInitServer(server_local.get());
    return server_local;
}

void VNCServer::captureScreenshot() {
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
    cv::Mat resized_img{server->height, server->width, CV_8UC4, server->frameBuffer};
    cv::resize(img, resized_img, cv::Size(server->width, server->height), 0, 0, cv::INTER_LINEAR);
}

void VNCServer::drawCursor(cv::Mat &img ) {
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


void VNCServer::handlePointerEvent(int buttonMask, int x, int y, rfbClientPtr) {
    spdlog::info("Pointer event, mask: {}, x: {}, y: {}", buttonMask, x, y);

    x += screens->x_org;
    y += screens->y_org;

    XTestFakeMotionEvent(display.get(), -1, x, y, CurrentTime);
    if (buttonMask!=0) {
        unsigned int button = extractButtonID(buttonMask);
        constexpr int is_clicked_bit = 8;
        bool is_clicked = buttonMask & 1 << (is_clicked_bit - 1);
        spdlog::info("click: {}, button: {}", is_clicked, button);
        XTestFakeButtonEvent(display.get(), button, is_clicked, CurrentTime);
    }
    XFlush(display.get());
}

unsigned int VNCServer::extractButtonID(int buttonMask) const {
    if (buttonMask & rfbButton1Mask) return Button1;
    if (buttonMask & rfbButton2Mask) return Button2;
    if (buttonMask & rfbButton3Mask) return Button3;
    if (buttonMask & rfbButton4Mask) return Button4;
    if (buttonMask & rfbButton5Mask) return Button5;
    return 0;
}

void VNCServer::handleKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr) {
    KeyCode keycode = XKeysymToKeycode(display.get(), key);
    spdlog::info("Got keycode: {}");
    if (keycode == 0) {
        std::cerr << "Invalid key symbol" << std::endl;
        return;
    }

    XTestFakeKeyEvent(display.get(), keycode, down, CurrentTime);
    XFlush(display.get());
}

std::unique_ptr<XineramaScreenInfo, decltype(&XFree)> VNCServer::getScreensInfo() {
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

std::unique_ptr<char[]> VNCServer::allocateBuffer() {
    return std::make_unique<char[]>(
            static_cast<std::size_t>(screens->width) * static_cast<std::size_t>(screens->height) *
            PIXEL_WIDTH);
}
