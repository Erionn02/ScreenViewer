#pragma once
#include "ScreenViewerBaseException.hpp"

#include <SDL2/SDL.h>
#include <rfb/rfbclient.h>

#include <string>
#include <chrono>

class VNCClientException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


class VNCClient {
public:
    VNCClient(std::string server_ip);
    VNCClient(VNCClient&&) = default;
    ~VNCClient();

    void run();

private:
    void handleIOEvents(std::chrono::milliseconds max_poll_time, std::chrono::milliseconds mouse_position_update_interval);
    std::unique_ptr<rfbClient, decltype(&rfbClientCleanup)> createClient(std::string server_ip);
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> createWindow();
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> createRenderer();
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> createTexture();


    int window_width{800};
    int window_height{600};
    std::unique_ptr<rfbClient, decltype(&rfbClientCleanup)> client;
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window;
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer;
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture;
};


