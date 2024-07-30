#pragma once

#include "Message.hpp"

#include <opencv2/core.hpp>

class IOControllerException: ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


class IOController {
public:
    virtual ~IOController() = default;
    virtual void handleKeyboardEvent(KeyboardEventData event_data) = 0;
    virtual void handleMouseEvent(MouseEventData event_data) = 0;
    virtual cv::Mat captureScreenshot() = 0;

protected:
    void drawCursor(cv::Mat &screenshot, cv::Mat &cursor, cv::Point offset);
};