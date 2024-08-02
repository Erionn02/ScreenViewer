#pragma once
#include "ScreenViewerBaseException.hpp"
#include "Message.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <future>

#include <optional>


using boost::asio::ip::tcp;

class SocketException : public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


// Every message is preceded by header that contains amount of bytes to send
class SocketBase : public std::enable_shared_from_this<SocketBase> {
public:
    SocketBase(boost::asio::ssl::stream<tcp::socket> socket_);
    SocketBase(SocketBase&&) = default;

    virtual ~SocketBase() = default;

    using MessageHandler = std::function<void(BorrowedMessage)>;
    void asyncReadMessage(MessageHandler message_handler, std::size_t max_message_size = BUFFER_SIZE);

    // User has to ensure that message's content lives until it's successfully sent.
    template <typename Callable = decltype([]{})>
    void asyncSendMessage(BorrowedMessage &message, Callable&& completion_handler = {}) {
        MessageHeader header{.message_size = message.content.size(),
                .type = message.type};
        std::vector<char> serialized_header{std::bit_cast<char *>(&header), std::bit_cast<char *>(&header) + sizeof(header)};
        std::vector<boost::asio::const_buffer> message_with_header{};
        message_with_header.emplace_back(boost::asio::buffer(serialized_header.data(), serialized_header.size()));
        message_with_header.emplace_back(boost::asio::buffer(message.content));

        async_write(socket_, message_with_header,
                    [self = shared_from_this(),
                            serialized_header = std::move(serialized_header),
                            completion_handler = std::forward<Callable>(completion_handler)] (boost::system::error_code ec, size_t) mutable {
                        if(!ec) {
                            completion_handler();
                        }
                    });
    }
    void disconnect(std::optional<std::string> disconnect_msg);

    void send(const OwnedMessage &message);
    void send(BorrowedMessage message);

    template<Trivial MessageTypeData>
    void send(MessageType type, MessageTypeData data) {
        send(BorrowedMessage {.type = type,
                              .content = {std::bit_cast<char*>(&data), sizeof(data)}});
    }

    OwnedMessage receive();
    BorrowedMessage receiveToBuffer();

    boost::asio::ssl::stream<tcp::socket>& getSocket();

    void receiveACK();
    void sendACK();
    void sendNACK();
    std::string_view getBuffer();
    bool isOpen();
protected:
    void sendChunk(BorrowedMessage message);
    MessageHeader readHeader() ;
    void asyncReadHeader(MessageHandler message_handler, std::size_t max_message_size);
    void asyncReadMessageImpl(std::shared_ptr<SocketBase> self, MessageHandler message_handler,
                              MessageHeader header);

    void safeDisconnect(std::optional<std::string> disconnect_msg);


    boost::asio::ssl::stream<tcp::socket> socket_;
    std::unique_ptr<char[]> data_buffer;
public:
    static constexpr std::size_t BUFFER_SIZE{1024 * 1024 * 5}; // 5 MiB
};