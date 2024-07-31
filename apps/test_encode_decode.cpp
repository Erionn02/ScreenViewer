#include <opencv2/opencv.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "streamer/X11IOController.hpp"


struct ctxFree {
    void operator()(AVCodecContext * ctx) {
        avcodec_free_context(&ctx);
    }
};

class Encoder {
public:
    Encoder(int fps, int height, int width): context(createEncodeContext(fps, height, width)), frame(createFrame(context.get()),
                                                                                                     av_frame_unref) {}

    std::unique_ptr<AVPacket, decltype(&av_packet_unref)> encode(cv::Mat mat) {
        std::unique_ptr<AVPacket, decltype(&av_packet_unref)> packet{nullptr, av_packet_unref};
        cvmatToAvframe(mat, frame.get());
        frame->pts = frame_id;
        int ret;
        if ((ret = avcodec_send_frame(context.get(), frame.get())) == 0) {
            std::cout<< "Frame "<< frame_id << " sent!" << std::endl;
            frame_id = (frame_id % context->framerate.num) + 1;
        } else if (ret == AVERROR(EAGAIN)) {
            std::cout<< "Failed to send frame, ret: " << ret << std::endl;
            packet = {av_packet_alloc(), av_packet_unref};
            ret = avcodec_receive_packet(context.get(), packet.get());
            std::cout<< "Receive packet, ret: " << ret << std::endl;
            std::cout<< "Packet size: " << packet->size << std::endl;
        }

        return packet;
    }


private:
    void cvmatToAvframe(cv::Mat& image, AVFrame *frame) {
        int width = image.cols;
        int height = image.rows;
        int cvLinesizes[1];
        cvLinesizes[0] = image.step1();
        SwsContext *conversion = sws_getContext(
                width, height, AVPixelFormat::AV_PIX_FMT_BGR24, width, height,
                (AVPixelFormat)frame->format, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        sws_scale(conversion, &image.data, cvLinesizes, 0, height, frame->data,
                  frame->linesize);
        sws_freeContext(conversion);
    }


    AVFrame *createFrame(const AVCodecContext *context) {
        AVFrame *frame_ptr = av_frame_alloc();
        if (!frame_ptr){
            std::cerr << "Could not allocate video frame_ptr" << std::endl;
            exit(7);
        }

        frame_ptr->format = context->pix_fmt;
        frame_ptr->height = context->height;
        frame_ptr->width  = context->width;

        if(av_frame_get_buffer(frame_ptr, 0) < 0){
            std::cerr << "Can't allocate the video frame_ptr data" << std::endl;
            exit(8);
        }

        if (av_frame_make_writable(frame_ptr) < 0){
            std::cerr << "Frame data not writable" << std::endl;
            exit(9);
        }

        return frame_ptr;
    }

    AVCodecContext *createEncodeContext(int fps, int height, int width) {
        auto codec = avcodec_find_encoder(AV_CODEC_ID_H265);
        if (!codec){
            std::cerr << "Codec with specified id not found" << std::endl;
            exit(1);
        }
        auto context_ptr = avcodec_alloc_context3(codec);
        if (!context_ptr){
            std::cerr << "Can't allocate video codec context_ptr" << std::endl;
            exit(2);
        }

        context_ptr->height = height;
        context_ptr->width = width;

        context_ptr->time_base.num = 1;
        context_ptr->time_base.den = fps;
        context_ptr->framerate.num = fps;
        context_ptr->framerate.den = 1;

        context_ptr->pix_fmt = AV_PIX_FMT_YUV420P;

        context_ptr->gop_size = fps * 2;
//    context_ptr->max_b_frames = 3;
//    context_ptr->refs = 3;

        av_opt_set(context_ptr->priv_data, "preset", "ultrafast", 0);
        av_opt_set(context_ptr->priv_data, "crf", "35", 0);
        av_opt_set(context_ptr->priv_data, "tune", "zerolatency", 0);


        auto desc = av_pix_fmt_desc_get(AV_PIX_FMT_RGB24);
        if (!desc){
            std::cerr << "Can't get descriptor for pixel format" << std::endl;
            exit(3);
        }
        auto bytesPerPixel = av_get_bits_per_pixel(desc) / 8;
        if(!(bytesPerPixel==3 && !(av_get_bits_per_pixel(desc) % 8))){
            std::cerr << "Unhandled bits per pixel, bad in pix fmt" << std::endl;
            exit(4);
        }

        if(avcodec_open2(context_ptr, codec, nullptr) < 0){
            std::cerr << "Could not open codec" << std::endl;
            exit(5);
        }
        return context_ptr;
    }


    std::size_t frame_id{1};
    std::unique_ptr<AVCodecContext, ctxFree> context;
    std::unique_ptr<AVFrame, decltype(&av_frame_unref)> frame;
};


class Decoder {
public:
    Decoder(): context(createDecodeContext()) {}

    cv::Mat decode(AVPacket* packet) {
        if (avcodec_send_packet(context.get(), packet) == 0) {
            if (avcodec_receive_frame(context.get(), frame.get()) == 0) {
                return avframeToCvmat();
            }
        }
        return {};
    }

private:
    AVCodecContext *createDecodeContext() {
        auto codec = avcodec_find_decoder(AV_CODEC_ID_H265);
        if (!codec){
            std::cerr << "Could not found codec by given id" << std::endl;
            exit(10);
        }

        auto context_ptr = avcodec_alloc_context3(codec);
        if (!context_ptr){
            std::cerr << "Could not allocate avcodec context_ptr" << std::endl;
            exit(11);
        }

        if (avcodec_open2(context_ptr, codec, nullptr) < 0){
            std::cerr << "Could not initialize avcodec context_ptr" << std::endl;
            exit(12);
        }
        auto desc = av_pix_fmt_desc_get(AV_PIX_FMT_RGB24);
        if (!desc){
            std::cerr << "Can't get descriptor for pixel format" << std::endl;
            exit(13);
        }
        auto bytesPerPixel = av_get_bits_per_pixel(desc) / 8;
        if(!(bytesPerPixel==3 && !(av_get_bits_per_pixel(desc) % 8) )){
            std::cerr << "Unhandled bits per pixel, bad in pix fmt" << std::endl;
            exit(14);
        }
        return context_ptr;
    }

    cv::Mat avframeToCvmat() {
        int width = frame->width;
        int height = frame->height;
        cv::Mat image(height, width, CV_8UC3);
        int cvLinesizes[1];
        cvLinesizes[0] = image.step1();
        SwsContext *conversion = sws_getContext(
                width, height, (AVPixelFormat)frame->format, width, height,
                AVPixelFormat::AV_PIX_FMT_BGR24, SWS_FAST_BILINEAR, NULL, NULL, NULL);
        sws_scale(conversion, frame->data, frame->linesize, 0, height, &image.data,
                  cvLinesizes);
        sws_freeContext(conversion);
        return image;
    }

    std::unique_ptr<AVFrame, decltype(&av_frame_unref)> frame{av_frame_alloc(), av_frame_unref};
    std::unique_ptr<AVCodecContext, ctxFree> context;
};



int main(int argc, char* argv[]) {
    X11IOController ss{};
    cv::Mat mat = ss.captureScreenshot(); // Replace this with actual frame capture
    cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);

    int fps = 30;
    int height = mat.rows;
    int width = mat.cols;



    Encoder encoder{fps, height, width};
    Decoder decoder{};

    while(true) {
        mat = ss.captureScreenshot();
        cv::cvtColor(mat, mat, cv::COLOR_BGRA2BGR);
        auto packet = encoder.encode(mat);
        if (packet) {
            auto result = decoder.decode(packet.get());
            if(!result.empty()) {
                std::cout << "Not empty." << std::endl;
                cv::imwrite("/tmp/out.png", result);
            }
        }
    }



    return 0;
}
