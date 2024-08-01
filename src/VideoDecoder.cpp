#include "VideoDecoder.hpp"


VideoDecoder::VideoDecoder(): context(createDecodeContext()) {}

cv::Mat VideoDecoder::decode(AVPacket *packet)  {
    auto ret = avcodec_send_packet(context.get(), packet);
    auto recv_frame_ret = avcodec_receive_frame(context.get(), frame.get());
    if (recv_frame_ret == 0) {
        return avframeToCvmat();
    }
    return {};
}

AVCodecContext *VideoDecoder::createDecodeContext() {
    auto codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec){
        throw VideoDecoderException("Could not find codec by given id.");
    }

    auto context_ptr = avcodec_alloc_context3(codec);
    if (!context_ptr){
        throw VideoDecoderException("Could not allocate avcodec context.");
    }

    if (avcodec_open2(context_ptr, codec, nullptr) < 0){
        throw VideoDecoderException("Could not initialize encode avcodec context.");
    }
    auto desc = av_pix_fmt_desc_get(AV_PIX_FMT_RGB24);
    if (!desc){
        throw VideoDecoderException("Could not get descriptor for pixel format");
    }

    if(av_get_bits_per_pixel(desc) != 3*8) {
        throw VideoDecoderException("Unhandled bits per pixel, bad in pix fmt.");
    }

    return context_ptr;
}

cv::Mat VideoDecoder::avframeToCvmat() {
    int new_width = frame->width;
    int new_height = frame->height;
    cv::Mat image(new_height, new_width, CV_8UC3);
    int cvLinesizes[]{static_cast<int>(image.step1())};
    SwsContext *conversion = sws_getContext(
            frame->width, frame->height, (AVPixelFormat)frame->format, new_width, new_height,
            AVPixelFormat::AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
    sws_scale(conversion, frame->data, frame->linesize, 0, new_height, &image.data,
              cvLinesizes);
    sws_freeContext(conversion);
    return image;
}
