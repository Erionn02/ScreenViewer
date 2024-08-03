#include "streamer/WaylandIOController.hpp"

#include <opencv2/imgcodecs.hpp>

#include <iostream>


int main() {

    WaylandIOController controller{};


    auto screenshot = controller.captureScreenshot();

    cv::imwrite("/tmp/wayland_screenshot.png", screenshot);
    system("xdg-open /tmp/wayland_screenshot.png");


}