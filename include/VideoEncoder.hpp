#pragma once

#include "ScreenViewerBaseException.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <spdlog/spdlog.h>
#include <opencv2/opencv.hpp>

class VideoEncoderException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


class VideoEncoder {
public:
    VideoEncoder(int fps, int height, int width);

    std::unique_ptr<AVPacket, decltype(&av_packet_unref)> encode(cv::Mat mat);
private:
    void convertToAVFrame(cv::Mat &image);
    AVFrame *createFrame();
    AVCodecContext *createEncodeContext(int fps, int height, int width);

    struct ctxFree {
        void operator()(AVCodecContext *ctx) {
            avcodec_free_context(&ctx);
        }
    };

    int frame_id{1};
    std::unique_ptr<AVCodecContext, ctxFree> context;
    std::unique_ptr<AVFrame, decltype(&av_frame_unref)> frame;
};
