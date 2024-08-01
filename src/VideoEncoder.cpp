#include "VideoEncoder.hpp"


VideoEncoder::VideoEncoder(int fps, int height, int width) : context(createEncodeContext(fps, height, width)),
                                                             frame(createFrame(), av_frame_unref) {}

std::unique_ptr<AVPacket, decltype(&av_packet_unref)> VideoEncoder::encode(cv::Mat mat)  {
    std::unique_ptr<AVPacket, decltype(&av_packet_unref)> packet{nullptr, av_packet_unref};
    convertToAVFrame(mat);

    frame->pts = frame_id;
    int ret;
    if ((ret = avcodec_send_frame(context.get(), frame.get())) == 0) {
        frame_id = (frame_id % context->framerate.num) + 1;
        packet = {av_packet_alloc(), av_packet_unref};
        ret = avcodec_receive_packet(context.get(), packet.get());
    } else if (ret == AVERROR(EAGAIN)) {
        std::cout << "Failed to send frame, EAGAIN" << std::endl;
        packet = {av_packet_alloc(), av_packet_unref};
        ret = avcodec_receive_packet(context.get(), packet.get());
        std::cout << "Receive packet, ret: " << ret << std::endl;
        std::cout << "Packet size: " << packet->size << std::endl;
    } else {
        std::cout << "Failed to send frame, ret" << ret << std::endl;
    }

    return packet;
}

void VideoEncoder::convertToAVFrame(cv::Mat &image) {
    int width = image.cols;
    int height = image.rows;
    int cvLinesizes[]{static_cast<int>(image.step1())};
    SwsContext *conversion = sws_getContext(
            width, height, AVPixelFormat::AV_PIX_FMT_BGR24, width, height,
            (AVPixelFormat) frame->format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    sws_scale(conversion, &image.data, cvLinesizes, 0, height, frame->data,
              frame->linesize);
    sws_freeContext(conversion);
}

AVFrame *VideoEncoder::createFrame() {
    AVFrame *frame_ptr = av_frame_alloc();
    if (!frame_ptr) {
        throw VideoEncoderException("Could not allocate video frame_ptr");
    }

    frame_ptr->format = context->pix_fmt;
    frame_ptr->height = context->height;
    frame_ptr->width = context->width;

    if (av_frame_get_buffer(frame_ptr, 0) < 0) {
        throw VideoEncoderException("Could not allocate the video frame data");
    }

    if (av_frame_make_writable(frame_ptr) < 0) {
        throw VideoEncoderException("Frame data not writable");
    }

    return frame_ptr;
}

AVCodecContext *VideoEncoder::createEncodeContext(int fps, int height, int width) {
    auto codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        throw VideoEncoderException("Codec with specified id not found.");
    }
    auto context_ptr = avcodec_alloc_context3(codec);
    if (!context_ptr) {
        throw VideoEncoderException("Could not allocate video codec context.");
    }

    context_ptr->height = height;
    context_ptr->width = width;

    context_ptr->time_base.num = 1;
    context_ptr->time_base.den = fps;
    context_ptr->framerate.num = fps;
    context_ptr->framerate.den = 1;

    context_ptr->pix_fmt = AV_PIX_FMT_YUV420P;

    context_ptr->gop_size = fps * 2;


    av_opt_set(context_ptr->priv_data, "preset", "ultrafast", 0);
    av_opt_set(context_ptr->priv_data, "crf", "30", 0);
    av_opt_set(context_ptr->priv_data, "tune", "zerolatency", 0);


    auto desc = av_pix_fmt_desc_get(AV_PIX_FMT_RGB24);
    if (!desc) {
        throw VideoEncoderException("Could get descriptor for pixel format");
    }
    if (av_get_bits_per_pixel(desc) != 3*8) {
        throw VideoEncoderException("Unhandled bits per pixel, bad in pix fmt");
    }

    if (avcodec_open2(context_ptr, codec, nullptr) < 0) {
        throw VideoEncoderException("Could not initialize decode avcodec context.");
    }
    return context_ptr;
}