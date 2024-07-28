#include <gtest/gtest.h>

#include "Message.hpp"


struct MessageHeaderTest : public testing::Test {
    std::size_t message_size = 10000;
    std::size_t message_size_limit = 2 * message_size;
    MessageHeader header{.message_size = message_size,
            .type = MessageType::ACK};
};

TEST_F(MessageHeaderTest, canDeserialize) {
    MessageHeader deserialized_header = MessageHeader::deserialize(std::bit_cast<const char *>(&header), sizeof(header),
                                                                   message_size_limit);

    ASSERT_EQ(header.type, deserialized_header.type);
    ASSERT_EQ(header.message_size, deserialized_header.message_size);
}

TEST_F(MessageHeaderTest, throwsWhenBufferIsTooSmall) {
    ASSERT_THROW(
            (MessageHeader::deserialize(std::bit_cast<const char *>(&header), sizeof(header) - 1, message_size_limit)),
            MessageHeaderException);
}

TEST_F(MessageHeaderTest, throwsWhenMessageLengthExceedsMaxLength) {
    ASSERT_THROW((MessageHeader::deserialize(std::bit_cast<const char *>(&header), sizeof(header), message_size / 2)),
                 MessageHeaderException);
}

TEST_F(MessageHeaderTest, throwsWhenTypeOfMessageIsUnknown) {
    char msg_header_buff[sizeof(MessageHeader)];

    *std::bit_cast<std::size_t *>(static_cast<char *>(msg_header_buff)) = message_size;
    using Enum_t = std::underlying_type_t<MessageType>;
    *std::bit_cast<Enum_t *>(static_cast<char *>(msg_header_buff) + sizeof(std::size_t)) = static_cast<Enum_t>(MessageType::MAX_VALUE) + 1;

    ASSERT_THROW((MessageHeader::deserialize(msg_header_buff, sizeof(msg_header_buff), message_size_limit)),
                 MessageHeaderException);
}

