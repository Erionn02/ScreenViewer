#include "ScreenViewerClient.hpp"
#include "KeysMapping.hpp"
#include "MouseConfig.hpp"

#include <spdlog/spdlog.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

ScreenViewerClient::ScreenViewerClient(ClientSocket socket) : socket(std::move(socket)), window(createWindow()),
                                                              renderer(createRenderer()), texture(createTexture(frame_width, frame_height)) {}

ScreenViewerClient::~ScreenViewerClient() {
    SDL_Quit();
}

void ScreenViewerClient::run() {
    spdlog::info("ScreenViewerClient started.");
    while (true) {
        auto msg = socket.receiveToBuffer();
        if (msg.type==MessageType::SCREEN_UPDATE) {
            cv::Mat resized_img = getNewFrame(msg);
            if(!resized_img.empty()) {
                SDL_RenderSetLogicalSize(renderer.get(), window_width, window_height);
                SDL_UpdateTexture(texture.get(), NULL, resized_img.data, static_cast<int>(resized_img.step1()));
                SDL_RenderClear(renderer.get());
                SDL_RenderCopy(renderer.get(), texture.get(), NULL, NULL);
                SDL_RenderPresent(renderer.get());
            }
        } else {
            spdlog::info("Unexpected message type: {}", MESSAGE_TYPE_TO_STR.at(msg.type));
        }


        handleIOEvents(5ms, 50ms);
    }
}

cv::Mat ScreenViewerClient::getNewFrame(BorrowedMessage msg) {
    AVPacket packet{};
    packet.data = std::bit_cast<uint8_t *>(msg.content.data());
    packet.size = static_cast<int>(msg.content.size());

    auto decoded_image = decoder.decode(&packet);

    if(!decoded_image.empty()) {
        if (decoded_image.rows != frame_height || decoded_image.cols != frame_width) {
            frame_height = decoded_image.rows;
            frame_width = decoded_image.cols;
            texture = createTexture(frame_width, frame_height);
            cv::Mat resized_img{};
            cv::resize(decoded_image, resized_img, cv::Size(frame_width, frame_height));
            return resized_img;
        }
    }
    return decoded_image;
}

void
ScreenViewerClient::handleIOEvents(std::chrono::milliseconds max_poll_time, std::chrono::milliseconds mouse_position_update_interval) {
    SDL_Event event{};
    auto start = std::chrono::high_resolution_clock::now();

    while (SDL_PollEvent(&event) && (std::chrono::high_resolution_clock::now() - start) < max_poll_time) {
        switch (event.type) {
            [[likely]] case SDL_MOUSEMOTION: {
                static auto last_pos_update = std::chrono::high_resolution_clock::now();
                if (std::chrono::high_resolution_clock::now() - last_pos_update > mouse_position_update_interval) {
                    int x = event.motion.x * frame_width / window_width;
                    int y = event.motion.y * frame_height / window_height;
                    int button_mask = Mouse::MOVE;
                    spdlog::info("MOVE: mask: {}.", button_mask);
                    socket.send(MessageType::MOUSE_INPUT, MouseEventData{.button_mask = button_mask,
                                                                         .x = x,
                                                                         .y = y});
                    last_pos_update = std::chrono::high_resolution_clock::now();
                }
                break;
            }
            case SDL_MOUSEWHEEL: {
                int button_mask = event.button.x > 0 ? Mouse::SCROLL_UP_MASK : Mouse::SCROLL_DOWN_MASK;
                spdlog::info("WHEEL: mask: {}.", button_mask);
                socket.send(MessageType::MOUSE_INPUT, MouseEventData{.button_mask = button_mask,
                        .x = 0,
                        .y = 0});
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                int x = event.button.x * frame_width / window_width;
                int y = event.button.y * frame_height / window_height;
                int button_mask = Mouse::convertToMask(event.button.button);
                if (event.type == SDL_MOUSEBUTTONDOWN) {
                    Mouse::setClicked(button_mask);
                }
                spdlog::info("CLICK: Button: {}, mask: {}, is_clicked: {}", event.button.button, button_mask, Mouse::isClicked(button_mask));
                socket.send(MessageType::MOUSE_INPUT, MouseEventData{.button_mask = button_mask, .x = x, .y = y});
                break;
            }
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto key_sym = SDLKeySymToX11(event.key.keysym.sym);
                bool is_key_down = event.type == SDL_KEYDOWN;
                socket.send(MessageType::KEYBOARD_INPUT, KeyboardEventData{.down = is_key_down, .key = key_sym});
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

std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> ScreenViewerClient::createWindow() {
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window_local{SDL_CreateWindow("VNC Client",
                                                                                            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                                                                            window_width, window_height,
                                                                                            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE), SDL_DestroyWindow};
    if (!window_local) {
        throw VNCClientException(fmt::format("Could not create window: {}", SDL_GetError()));
    }
    return window_local;
}

std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> ScreenViewerClient::createRenderer() {
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer_local{SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED),
                                                                                 SDL_DestroyRenderer};
    if (!renderer_local) {
        throw VNCClientException(fmt::format("Could not create renderer: {}", SDL_GetError()));
    }
    return renderer_local;
}

std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> ScreenViewerClient::createTexture(int width, int height) {
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> texture_local {SDL_CreateTexture(renderer.get(),
                                SDL_PIXELFORMAT_BGR24,
                                SDL_TEXTUREACCESS_STREAMING,
                                                                                                 width, height), SDL_DestroyTexture};
    if (!texture_local) {
        throw VNCClientException(fmt::format("Could not create texture: {}", SDL_GetError()));
    }
    return texture_local;
}
