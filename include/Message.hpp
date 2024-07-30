#pragma once

#include "ScreenViewerBaseException.hpp"

#include <fmt/format.h>

#include <cstddef>
#include <unordered_map>
#include <concepts>


enum class MessageType : unsigned char {
    JUST_A_MESSAGE,
    LOGIN,
    REGISTER_STREAMER,
    FIND_STREAMER,
    START_STREAM,
    RESPONSE,
    ID,
    ACK,
    NACK,
    MOUSE_INPUT,
    KEYBOARD_INPUT,
    SCREEN_UPDATE,
    DISCONNECT,

    MAX_VALUE = DISCONNECT
}; // sadly, C++ does not provide any type trait to obtain enum's max or min value, so we have to be careful here

const std::unordered_map<MessageType, std::string> MESSAGE_TYPE_TO_STR{
        {MessageType::JUST_A_MESSAGE,    "JUST_A_MESSAGE"},
        {MessageType::LOGIN,             "LOGIN"},
        {MessageType::REGISTER_STREAMER, "REGISTER_STREAMER"},
        {MessageType::FIND_STREAMER,     "FIND_STREAMER"},
        {MessageType::START_STREAM,      "START_STREAM"},
        {MessageType::RESPONSE,          "RESPONSE"},
        {MessageType::ID,                "ID"},
        {MessageType::ACK,               "ACK"},
        {MessageType::NACK,              "NACK"},
        {MessageType::MOUSE_INPUT,       "MOUSE_INPUT"},
        {MessageType::KEYBOARD_INPUT,    "KEYBOARD_INPUT"},
        {MessageType::SCREEN_UPDATE,     "SCREEN_UPDATE"},
        {MessageType::DISCONNECT,        "DISCONNECT"},
};

class MessageHeaderException : public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};

class MessageTypeException : public ScreenViewerBaseException {
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

struct KeyboardEventData {
    bool down;
    unsigned int key;

    bool operator==(const KeyboardEventData &other) const = default;
};

static_assert(sizeof(MessageHeader) == sizeof(std::size_t) + sizeof(MessageType));
static_assert(sizeof(KeyboardEventData) == sizeof(bool) + sizeof(unsigned int));
#pragma pack(pop)


template<typename Str_t>
struct Message {
    MessageType type;
    Str_t content;

    bool operator==(const Message &other) const = default;
};

using BorrowedMessage = Message<std::string_view>;
using OwnedMessage = Message<std::string>;


struct MouseEventData {
    int button_mask;
    int x;
    int y;

    bool operator==(const MouseEventData &other) const = default;
};

template<typename T>
concept Trivial = requires(T a){
    std::is_trivially_constructible_v<T>;
    std::is_trivially_copy_constructible_v<T>;
};

template<Trivial MessageTypeData, typename Str_t>
MessageTypeData convertTo(const Message<Str_t> &source) {
    if (source.content.size() != sizeof(MessageTypeData)) {
        throw MessageTypeException(
                fmt::format("Message's size ({}) != ({}) sizeof(MessageTypeData)", source.content.size(),
                            sizeof(MessageTypeData)));
    }
    return *std::bit_cast<MessageTypeData *>(source.content.data());
}