#pragma once
#include "ScreenViewerBaseException.hpp"
#include "MessageHeader.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <optional>


using boost::asio::ip::tcp;

class SocketException : public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};

template<typename Str_t>
struct Message {
    MessageType type;
    Str_t content;

    bool operator==(const Message& other) const {
        return type == other.type && content == other.content;
    };
};

using BorrowedMessage = Message<std::string_view>;
using OwnedMessage = Message<std::string>;


// Every message is preceded by header that contains amount of bytes to send
class SocketBase : public std::enable_shared_from_this<SocketBase> {
public:
    SocketBase(boost::asio::ssl::stream<tcp::socket> socket_);
    SocketBase(SocketBase&&) = default;

    virtual ~SocketBase() = default;

    using MessageHandler = std::function<void(BorrowedMessage)>;
    void asyncReadMessage(MessageHandler message_handler, std::size_t max_message_size = BUFFER_SIZE);
    void disconnect(std::optional<std::string> disconnect_msg);

    void send(const OwnedMessage &message);
    void send(BorrowedMessage message);

    OwnedMessage receive();
    BorrowedMessage receiveToBuffer();

    boost::asio::ssl::stream<tcp::socket>& getSocket();

    void receiveACK();
    void sendACK();

    std::string_view getBuffer();
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
    static constexpr std::size_t BUFFER_SIZE{1024 * 1024 * 1}; // 1 MiB
};