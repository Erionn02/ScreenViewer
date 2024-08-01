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


class VideoDecoderException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


class VideoDecoder {
public:
    VideoDecoder();

    cv::Mat decode(AVPacket* packet);
private:
    AVCodecContext *createDecodeContext();
    cv::Mat avframeToCvmat();

    struct ctxFree {
        void operator()(AVCodecContext * ctx) {
            avcodec_free_context(&ctx);
        }
    };

    std::unique_ptr<AVFrame, decltype(&av_frame_unref)> frame{av_frame_alloc(), av_frame_unref};
    std::unique_ptr<AVCodecContext, ctxFree> context;
};

