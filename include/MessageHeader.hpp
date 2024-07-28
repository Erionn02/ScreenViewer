#pragma once

#include <cstddef>

#include "ScreenViewerBaseException.hpp"

enum class MessageType : unsigned char {
    JUST_A_MESSAGE,
    LOGIN,
    START_STREAM,
    STREAM_CONNECT,
    ACK,
    NACK,
    MOUSE_INPUT,
    KEYBOARD_INPUT,
    SCREEN_UPDATE,
    DISCONNECT,

    MAX_VALUE  = DISCONNECT
}; // sadly, C++ does not provide any type trait to obtain enum's max or min value, so we have to be careful here

class MessageHeaderException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


#pragma pack(push)
#pragma pack(1) // disable padding
struct MessageHeader {
    std::size_t message_size;
    MessageType type;

    static MessageHeader deserialize(const char *buffer, std::size_t buffer_size, std::size_t max_length);
};

static_assert(sizeof(MessageHeader) == sizeof(std::size_t) + sizeof(MessageType));
#pragma pack(pop)