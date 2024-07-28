#include "Message.hpp"

#include <fmt/format.h>

#include <bit>

MessageHeader MessageHeader::deserialize(const char *buffer, std::size_t buffer_size, std::size_t max_length) {
    if (buffer_size < sizeof(MessageHeader)) {
        throw MessageHeaderException(fmt::format("Buffer is too short to deserialize from ({} < sizeof(MessageHeader)).", buffer_size));
    }
    std::size_t length = *std::bit_cast<std::size_t*>(buffer);
    using Enum_t = std::underlying_type_t<MessageType>;
    auto declared_msg_type = *std::bit_cast<Enum_t *>(buffer + sizeof(length));
    if (length > max_length) {
        throw MessageHeaderException(fmt::format("Message is too long ({} > {}).", length, max_length));
    }

    // Cpp standard states that casting a value (to enum) that does not represent any enumeration is UB
    // therefore I have to check it first and make it a little ugly in the process.
    if (declared_msg_type > static_cast<Enum_t>(MessageType::MAX_VALUE)) {
        throw MessageHeaderException(fmt::format("Got unknown message type ({}).", declared_msg_type));
    }
    return {length, static_cast<MessageType>(declared_msg_type)};
}
