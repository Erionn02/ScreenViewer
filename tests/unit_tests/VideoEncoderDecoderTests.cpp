#include <gtest/gtest.h>

#include "VideoDecoder.hpp"
#include "VideoEncoder.hpp"


TEST(VideoEncoderDecoderTests, canEncodeAndDecodeScreenshots) {
    auto screenshot = cv::imread(TEST_DIR"/test_screenshot.png", cv::IMREAD_UNCHANGED);
    int fps{30};
    int height{screenshot.rows};
    int width{screenshot.cols};

    VideoEncoder encoder{fps, height, width};
    VideoDecoder decoder{};

    auto packet = encoder.encode(screenshot);
    ASSERT_TRUE(packet);

    auto decoded_img = decoder.decode(packet.get());

    ASSERT_EQ(decoded_img.rows, screenshot.rows);
    ASSERT_EQ(decoded_img.cols, screenshot.cols);
    ASSERT_EQ(decoded_img.channels(), 3);
    // it's hard to make any assertions about the content of images, but those are better than none
}