#include <opencv2/opencv.hpp>
#include <rfb/rfb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xfixes.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <spdlog/spdlog.h>

void drawCursor(cv::Mat& img, XFixesCursorImage* cursorImage, int cursorX, int cursorY) {
    int imgWidth = img.cols;
    int imgHeight = img.rows;
    int bpp = img.channels();

    for (int y = 0; y < cursorImage->height; y++) {
        for (int x = 0; x < cursorImage->width; x++) {
            int dstX = cursorX + x - cursorImage->xhot;
            int dstY = cursorY + y - cursorImage->yhot;

            if (dstX >= 0 && dstX < imgWidth && dstY >= 0 && dstY < imgHeight) {
                unsigned long pixel = cursorImage->pixels[y * cursorImage->width + x];
                unsigned char alpha = (pixel >> 24) & 0xFF;
                unsigned char red = (pixel >> 16) & 0xFF;
                unsigned char green = (pixel >> 8) & 0xFF;
                unsigned char blue = pixel & 0xFF;

                if (alpha > 0) {
                    cv::Vec4b& bgPixel = img.at<cv::Vec4b>(dstY, dstX);
                    if (alpha == 255) {
                        bgPixel = cv::Vec4b(blue, green, red, alpha);
                    } else {
                        bgPixel[0] = static_cast<unsigned char>((blue * alpha + bgPixel[0] * (255 - alpha)) / 255);
                        bgPixel[1] = static_cast<unsigned char>((green * alpha + bgPixel[1] * (255 - alpha)) / 255);
                        bgPixel[2] = static_cast<unsigned char>((red * alpha + bgPixel[2] * (255 - alpha)) / 255);
                        bgPixel[3] = static_cast<unsigned char>(alpha + bgPixel[3] * (255 - alpha) / 255);
                    }
                }
            }
        }
    }
}

void captureScreen(rfbScreenInfoPtr server) {
    static Display* display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    if (!XineramaIsActive(display)) {
        fprintf(stderr, "Xinerama is not active\n");
        XCloseDisplay(display);
        exit(1);
    }

    int screenCount;
    static XineramaScreenInfo* screens = XineramaQueryScreens(display, &screenCount);
    if (!screens) {
        fprintf(stderr, "Failed to query Xinerama screens\n");
        XCloseDisplay(display);
        exit(1);
    }

    // Assuming the first screen is the primary screen
    int screenWidth = screens[0].width;
    int screenHeight = screens[0].height;
    int screenX = screens[0].x_org;
    int screenY = screens[0].y_org;

    int serverWidth = server->width;
    int serverHeight = server->height;

    XImage* image = XGetImage(display, RootWindow(display, DefaultScreen(display)),
                              screenX, screenY, static_cast<unsigned int>(screenWidth),
                              static_cast<unsigned int>(screenHeight), AllPlanes, ZPixmap);

    if (!image) {
        fprintf(stderr, "Failed to capture screen\n");
        exit(1);
    }

    cv::Mat img = cv::Mat(screenHeight, screenWidth, CV_8UC4, image->data);
    XFixesCursorImage* cursorImage = XFixesGetCursorImage(display);
    if (cursorImage) {
        int cursorX = cursorImage->x;
        int cursorY = cursorImage->y;
        drawCursor(img, cursorImage, cursorX, cursorY);
        XFree(cursorImage);
    }
    cv::Mat resizedImg{serverHeight, serverWidth, CV_8UC4, server->frameBuffer};
    cv::resize(img, resizedImg, cv::Size(serverWidth, serverHeight), 0, 0, cv::INTER_LINEAR);

    XDestroyImage(image);
}

int main(int argc, char** argv) {
    // set to actual screen resolution to ensure best possible quality
    rfbScreenInfoPtr server = rfbGetScreen(&argc, argv, 1920, 1080, 8, 3, 4);
    if (!server) {
        fprintf(stderr, "Failed to initialize VNC server\n");
        return 1;
    }

    server->frameBuffer = (char*)malloc(1920 * 1080 * 4);
    server->alwaysShared = TRUE;

    rfbInitServer(server);

    fprintf(stderr, "VNC server started\n");

    while (rfbIsActive(server)) {
//        auto now = std::chrono::high_resolution_clock::now();
        captureScreen(server);
//        auto then = std::chrono::high_resolution_clock::now();
//        spdlog::info("Resize time: {}ms", std::chrono::duration_cast<std::chrono::milliseconds>(then - now).count());
        rfbMarkRectAsModified(server, 0, 0, server->width, server->height);
        rfbProcessEvents(server, 10000);
    }

    rfbScreenCleanup(server);
    return 0;
}
