#include "vnc/VNCClient.hpp"
#include "KeysMapping.hpp"
#include "MouseConfig.hpp"

#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

VNCClient::VNCClient(std::string server_ip) : client(createClient(std::move(server_ip))), window(createWindow()),
                                              renderer(createRenderer()), texture(createTexture()) {}

VNCClient::~VNCClient() {
    SDL_Quit();
}

void VNCClient::run() {
    while (true) {
        SDL_RenderSetLogicalSize(renderer.get(), window_width, window_height);
        if (WaitForMessage(client.get(), 100000) < 0) {
            throw VNCClientException("WaitForMessage failed");
        }
        if (!HandleRFBServerMessage(client.get())) {
            throw VNCClientException("HandleRFBServerMessage failed");
        }

        constexpr int PIXEL_WIDTH{4};
        SDL_UpdateTexture(texture.get(), NULL, client->frameBuffer, client->width * PIXEL_WIDTH);
        SDL_RenderClear(renderer.get());
        SDL_RenderCopy(renderer.get(), texture.get(), NULL, NULL);
        SDL_RenderPresent(renderer.get());

        handleIOEvents(5ms, 50ms);
    }
}

void
VNCClient::handleIOEvents(std::chrono::milliseconds max_poll_time, std::chrono::milliseconds mouse_position_update_interval) {
    SDL_Event event{};
    auto start = std::chrono::high_resolution_clock::now();

    while (SDL_PollEvent(&event) && (std::chrono::high_resolution_clock::now() - start) < max_poll_time) {
        switch (event.type) {
            [[likely]] case SDL_MOUSEMOTION: {
                static auto last_pos_update = std::chrono::high_resolution_clock::now();
                if (std::chrono::high_resolution_clock::now() - last_pos_update > mouse_position_update_interval) {
                    int x = event.motion.x * client->width / window_width;
                    int y = event.motion.y * client->height / window_height;
                    SendPointerEvent(client.get(), x, y, Mouse::MOVE_MASK);
                    last_pos_update = std::chrono::high_resolution_clock::now();
                }
                break;
            }
            case SDL_MOUSEWHEEL: {
                int button_mask = event.button.x > 0 ? Mouse::SCROLL_UP_MASK : Mouse::SCROLL_DOWN_MASK;
                SendPointerEvent(client.get(), 0, 0, button_mask);
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                int x = event.button.x * client->width / window_width;
                int y = event.button.y * client->height / window_height;
                int buttonMask = Mouse::convertToMask(event.button.button);
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    Mouse::setClicked(buttonMask);
                }
                SendPointerEvent(client.get(), x, y, buttonMask);
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto key_sym = SDLKeySymToRfbKeySym(event.key.keysym.sym);
                rfbBool down = (event.type == SDL_KEYDOWN) ? TRUE : FALSE;
                SendKeyEvent(client.get(), key_sym, down);
                break;
            }
            [[unlikely]] case SDL_WINDOWEVENT: {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    window_width = event.window.data1;
                    window_height = event.window.data2;
                }
                break;
            }
            [[unlikely]] case SDL_QUIT: {
                exit(1);
            }
        }
    }
}


std::unique_ptr<rfbClient, decltype(&rfbClientCleanup)> VNCClient::createClient(std::string server_ip) {
    std::unique_ptr<rfbClient, decltype(&rfbClientCleanup)> client_local = {rfbGetClient(8, 3, 4), rfbClientCleanup};
    if (!client_local) {
        throw VNCClientException("Failed to create to rfbClient.");
    }
    client_local->canHandleNewFBSize = FALSE;
    client_local->appData.encodingsString = strdup("tight hextile zlib corre rre raw");
    client_local->appData.compressLevel = 9;
    client_local->appData.qualityLevel = 9;

    int argc = 2;
    std::string program_name = "VNCClient";
    char *argv[] = {program_name.data(), server_ip.data()};
    if (!rfbInitClient(client.get(), &argc, argv)) {
        throw VNCClientException("Failed to connect to VNC server");
    }
    spdlog::info("Connected to VNC server");


    return client_local;
}

std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> VNCClient::createWindow() {
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_local{SDL_CreateWindow("VNC Client",
                                                                                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                                                                            window_width, window_height,
                                                                                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE), SDL_DestroyWindow};
    if (!window_local) {
        throw VNCClientException(fmt::format("Could not create window: {}", SDL_GetError()));
    }
    return window_local;
}

std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> VNCClient::createRenderer() {
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer_local{SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED),
                                                                                 SDL_DestroyRenderer};
    if (!renderer_local) {
        throw VNCClientException(fmt::format("Could not create renderer: {}", SDL_GetError()));
    }
    return renderer_local;
}

std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> VNCClient::createTexture() {
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture_local {SDL_CreateTexture(renderer.get(),
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                client->width, client->height), SDL_DestroyTexture};
    if (!texture_local) {
        throw VNCClientException(fmt::format("Could not create texture: {}", SDL_GetError()));
    }
    return texture_local;
}
