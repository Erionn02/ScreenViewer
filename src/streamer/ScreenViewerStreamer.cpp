#include "streamer/ScreenViewerStreamer.hpp"

#include <spdlog/spdlog.h>

#include <chrono>

using namespace std::chrono_literals;


ScreenViewerStreamer::ScreenViewerStreamer(std::shared_ptr<ClientSocket> socket,
                                           std::unique_ptr<IOController> io_controller) : socket(std::move(socket)),
                                                                                          io_controller(std::move(
                                                                                                  io_controller)) {}

void ScreenViewerStreamer::run() {
    spdlog::info("VNC server started");
    scheduleAsyncReadMessage();
    while (true) {
        auto screenshot = io_controller->captureScreenshot();
        cv::imencode(".png", screenshot, frame_buffer);
        auto ec = socket->asyncSendMessage(OwnedMessage{.type = MessageType::SCREEN_UPDATE,
                .content = {std::bit_cast<char *>(frame_buffer.data()), frame_buffer.size()}});
        handleIOEvents();
        ec.get();
    }
}

void ScreenViewerStreamer::scheduleAsyncReadMessage() {
    socket->asyncReadMessage([this](BorrowedMessage message) {
        messages.push(OwnedMessage{.type = message.type,
                .content = {message.content.data(), message.content.size()}});
        scheduleAsyncReadMessage();
    });
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
