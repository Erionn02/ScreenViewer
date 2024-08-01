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
    std::future<boost::system::error_code> asyncSendMessage(BorrowedMessage &message);
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