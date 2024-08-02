#include "streamer/ScreenViewerStreamer.hpp"
#include "VideoEncoder.hpp"

#include <spdlog/spdlog.h>

#include <chrono>

using namespace std::chrono_literals;


ScreenViewerStreamer::ScreenViewerStreamer(std::shared_ptr<ClientSocket> socket,
                                           std::unique_ptr<IOController> io_controller) : socket(std::move(socket)),
                                                                                          io_controller(std::move(io_controller)),
                                                                                          encoder(createVideoEncoder()) {}

void ScreenViewerStreamer::run() {
    spdlog::info("ScreenViewerStreamer started");
    scheduleAsyncPollIOEvents();
    scheduleAsyncSendScreenshots();
    while (socket->isOpen() || !messages.empty()) {
        handleIOEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds (1));
    }
}

void ScreenViewerStreamer::scheduleAsyncPollIOEvents() {
    socket->asyncReadMessage([this](BorrowedMessage message) {
        messages.push(OwnedMessage{.type = message.type,
                .content = {message.content.data(), message.content.size()}});
        scheduleAsyncPollIOEvents();
    });
}

void ScreenViewerStreamer::scheduleAsyncSendScreenshots() {
    cv::Mat screenshot = io_controller->captureScreenshot();
    cv::cvtColor(screenshot, screenshot, cv::COLOR_BGRA2BGR);
    auto packet = encoder.encode(screenshot);
    if(packet) {
        spdlog::info("Packet size: {}, flags: {}", packet->size, packet->flags);
        BorrowedMessage msg{.type = MessageType::SCREEN_UPDATE,
                .content{std::bit_cast<char*>(packet->data), static_cast<std::size_t>(packet->size)}};
        socket->asyncSendMessage(msg, [this, packet = std::move(packet)]{
            scheduleAsyncSendScreenshots();
        });
    }
}

void ScreenViewerStreamer::handleIOEvents() {
    OwnedMessage message{};
    while (messages.try_pop(message)) {
        handleInput(message);
    }
}

void ScreenViewerStreamer::handleInput(const OwnedMessage &message) {
    switch (message.type) {
        case MessageType::KEYBOARD_INPUT: {
            io_controller->handleKeyboardEvent(convertTo<KeyboardEventData>(message));
            break;
        }
        case MessageType::MOUSE_INPUT: {
            io_controller->handleMouseEvent(convertTo<MouseEventData>(message));
            break;
        }
        default: {
            spdlog::info("Got unexpected message type: {}", MESSAGE_TYPE_TO_STR.at(message.type));
            break;
        }
    }
}

VideoEncoder ScreenViewerStreamer::createVideoEncoder() {
    cv::Mat screenshot = io_controller->captureScreenshot();
    int fps = 30;
    int height = screenshot.rows;
    int width = screenshot.cols;
    return {fps, height, width};
}
