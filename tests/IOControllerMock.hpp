#include <gmock/gmock.h>

#include "streamer/IOController.hpp"

class IOControllerMock: public IOController {
public:
    MOCK_METHOD(void, handleKeyboardEvent, (KeyboardEventData), (override));
    MOCK_METHOD(void, handleMouseEvent, (MouseEventData), (override));
    MOCK_METHOD(cv::Mat, captureScreenshot, (), (override));
};