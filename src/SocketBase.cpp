#include "SocketBase.hpp"

#include <fmt/format.h>
#include <spdlog/spdlog.h>


namespace asio = boost::asio;
using boost::system::error_code;

SocketBase::SocketBase(boost::asio::ssl::stream<tcp::socket> socket_) : socket_(std::move(socket_)),
                                                                        data_buffer(
                                                                                std::make_unique<char[]>(
                                                                                        BUFFER_SIZE)) {
    static_assert(BUFFER_SIZE >= sizeof(MessageHeader));
}

void SocketBase::send(const OwnedMessage &message) {
    send(BorrowedMessage{.type = message.type, .content = message.content});
}

void SocketBase::send(BorrowedMessage message) {
    std::size_t message_length = message.content.size();
    std::size_t ptr_cursor = 0;
    while (message_length > BUFFER_SIZE) {
        sendChunk({message.type, {message.content.data() + ptr_cursor, BUFFER_SIZE}});
        message_length -= BUFFER_SIZE;
        ptr_cursor += BUFFER_SIZE;
    }
    sendChunk({message.type, {message.content.data() + ptr_cursor, message_length}});
}

void SocketBase::sendChunk(BorrowedMessage message) {
    MessageHeader header{.message_size = message.content.size(),
            .type = message.type};
    std::vector<boost::asio::const_buffer> message_with_header{};
    message_with_header.emplace_back(boost::asio::buffer(&header, sizeof(header)));
    message_with_header.push_back(boost::asio::buffer(message.content));
    boost::asio::write(socket_, message_with_header);
}

void SocketBase::disconnect(std::optional<std::string> disconnect_msg) {
    spdlog::debug("Disconnecting... {}", disconnect_msg.value_or(""));
    if (disconnect_msg.has_value()) {
        send(BorrowedMessage{MessageType::DISCONNECT, *disconnect_msg});
    }
    socket_.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_send);
}

OwnedMessage SocketBase::receive() {
    MessageHeader header = readHeader();
    std::string message{};
    message.resize(header.message_size);
    boost::asio::read(socket_, boost::asio::buffer(message), boost::asio::transfer_exactly(header.message_size));
    return {header.type, message};
}

MessageHeader SocketBase::readHeader() {
    char buf[sizeof(MessageHeader)];
    boost::asio::mutable_buffer buffer{buf, sizeof(buf)};
    boost::asio::read(socket_, buffer, boost::asio::transfer_exactly(buffer.size()));
    return MessageHeader::deserialize(buf, sizeof(buf), BUFFER_SIZE);
}

BorrowedMessage SocketBase::receiveToBuffer() {
    char buf[sizeof(MessageHeader)];
    boost::asio::mutable_buffer buffer{buf, sizeof(buf)};
    boost::asio::read(socket_, buffer, boost::asio::transfer_exactly(buffer.size()));
    MessageHeader header = MessageHeader::deserialize(buf, sizeof(buf), BUFFER_SIZE);
    boost::asio::read(socket_, boost::asio::mutable_buffer(data_buffer.get(), header.message_size),
                      boost::asio::transfer_exactly(header.message_size));
    return {header.type, {data_buffer.get(), header.message_size}};
}

void SocketBase::asyncReadMessage(MessageHandler message_handler, std::size_t max_message_size) {
    if (max_message_size > BUFFER_SIZE) {
        throw SocketException(
                fmt::format("Tried to schedule receiving message with max size of {} bytes, where buffer size is {}.",
                            max_message_size, BUFFER_SIZE));
    }
    asyncReadHeader(std::move(message_handler), max_message_size);
}

void SocketBase::asyncReadHeader(MessageHandler message_handler, std::size_t max_message_size) {
    constexpr std::size_t HEADER_SIZE = sizeof(MessageHeader);
    boost::asio::async_read(socket_, asio::buffer(data_buffer.get(), HEADER_SIZE),
                            asio::transfer_exactly(HEADER_SIZE),
                            [this, self = shared_from_this(), max_message_size, message_handler = std::move(
                                    message_handler)](error_code ec, std::size_t transferred_bytes) mutable {
                                if (!ec) {
                                    try  {
                                        MessageHeader header = MessageHeader::deserialize(data_buffer.get(), transferred_bytes, max_message_size);
                                        asyncReadMessageImpl(std::move(self), std::move(message_handler), header);
                                    } catch (const MessageHeaderException& e) {
                                        spdlog::warn(e.what());
                                        safeDisconnect(e.what());
                                        return;
                                    }
                                } else {
                                    spdlog::debug("Encountered an error during async read, aborting. Details: {}",
                                                  ec.what());
                                }
                            });
}

void
SocketBase::asyncReadMessageImpl(std::shared_ptr<SocketBase> self, MessageHandler message_handler,
                                 MessageHeader header) {
    boost::asio::async_read(socket_, asio::buffer(data_buffer.get(), header.message_size),
                            asio::transfer_exactly(header.message_size),
                            [self = std::move(self), message_handler = std::move(
                                    message_handler), this, header](error_code ec,
                                                            std::size_t message_size) {
                                if (!ec) {
                                    try {
                                        message_handler({header.type, {data_buffer.get(),message_size}});
                                    } catch(const std::exception& e) {
                                        spdlog::error("Encountered an error during handling message, aborting. Details: {}", e.what());
                                        safeDisconnect(e.what());
                                    }
                                } else {
                                    spdlog::error(
                                            "Encountered an error during async read, aborting. Details: {}",
                                            ec.what());
                                }
                            });
}

void SocketBase::receiveACK() {
    BorrowedMessage message = receiveToBuffer();
    if (message.type != MessageType::ACK) {
        std::string_view additional_message;
        constexpr std::size_t MAX_PRINTABLE_STR_LENGTH{100}; // totally arbitrary number
        if (message.content.size() < MAX_PRINTABLE_STR_LENGTH) {
            additional_message = message.content;
        } else {
            additional_message = "Too large to print.";
        }
        throw SocketException(
                fmt::format("Did not get ACK. Response type: {} size: {}. {}",
                            MESSAGE_TYPE_TO_STR.at(message.type), message.content.size(),
                            additional_message));
    }
}

void SocketBase::sendACK() {
    send(BorrowedMessage{MessageType::ACK, {}});
}

void SocketBase::sendNACK() {
    send(BorrowedMessage{MessageType::NACK, {}});
}

std::string_view SocketBase::getBuffer() {
    return {data_buffer.get(), BUFFER_SIZE};
}

void SocketBase::safeDisconnect(std::optional<std::string> disconnect_msg) {
    try {
        disconnect(std::move(disconnect_msg));
    } catch (const std::exception &e) {
        spdlog::warn(e.what());
    }
}

boost::asio::ssl::stream<tcp::socket> &SocketBase::getSocket() {
    return socket_;
}
