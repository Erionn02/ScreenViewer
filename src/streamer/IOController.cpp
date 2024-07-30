#include "streamer/IOController.hpp"

void IOController::drawCursor(cv::Mat &screenshot, cv::Mat &cursor, cv::Point offset) {
    for (int y = 0; y < cursor.rows; y++) {
        for (int x = 0; x < cursor.cols; x++) {
            int dstX = offset.x + x;
            int dstY = offset.y + y;

            if (dstX >= 0 && dstX < screenshot.cols && dstY >= 0 && dstY < screenshot.rows) {
                unsigned long pixel = cursor.data[y * cursor.cols + x];
                unsigned char alpha = (pixel >> 24) & 0xFF;
                unsigned char red = (pixel >> 16) & 0xFF;
                unsigned char green = (pixel >> 8) & 0xFF;
                unsigned char blue = pixel & 0xFF;

                if (alpha > 0) {
                    cv::Vec4b &bg_pixel = screenshot.at<cv::Vec4b>(dstY, dstX);
                    bg_pixel[0] = static_cast<unsigned char>((blue * alpha + bg_pixel[0] * (255 - alpha)) / 255);
                    bg_pixel[1] = static_cast<unsigned char>((green * alpha + bg_pixel[1] * (255 - alpha)) / 255);
                    bg_pixel[2] = static_cast<unsigned char>((red * alpha + bg_pixel[2] * (255 - alpha)) / 255);
                    bg_pixel[3] = static_cast<unsigned char>(alpha + bg_pixel[3] * (255 - alpha) / 255);
                }
            }
        }
    }
}
