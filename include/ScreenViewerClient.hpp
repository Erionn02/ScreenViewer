#pragma once
#include "ScreenViewerBaseException.hpp"
#include "ClientSocket.hpp"
#include "VideoDecoder.hpp"

#include <SDL2/SDL.h>

#include <string>
#include <chrono>
#include <opencv2/core/mat.hpp>

class VNCClientException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


class ScreenViewerClient {
public:
    ScreenViewerClient(ClientSocket socket);
    ScreenViewerClient(ScreenViewerClient&&) = default;
    ~ScreenViewerClient();

    void run();

private:
    void handleIOEvents(std::chrono::milliseconds max_poll_time, std::chrono::milliseconds mouse_position_update_interval);
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> createWindow();
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> createRenderer();
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> createTexture(int width, int height);
    cv::Mat getNewFrame(BorrowedMessage msg);


    int window_width{800};
    int window_height{600};

    int frame_width{window_width};
    int frame_height{window_height};

    ClientSocket socket;
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window;
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture;
    VideoDecoder decoder{};
};


