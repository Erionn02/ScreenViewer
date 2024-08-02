#pragma once

#include "SocketBase.hpp"

#include <thread>
#include <fstream>
#include <chrono>

class ClientSocketException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


class ClientSocket : public SocketBase {
public:
    ClientSocket(const std::string &host, unsigned short port, bool verify_cert = true);
    ClientSocket(ClientSocket&&) = default;
    ~ClientSocket();

    void login(const std::string &email, const std::string &password);
    bool findOtherClient(const std::string& id);
    std::string requestStreamerID();
    bool waitForStartStreamMessage(std::chrono::seconds timeout = std::chrono::seconds(std::numeric_limits<std::int64_t>::max()));
    void disconnect();
private:
    ClientSocket(std::shared_ptr<boost::asio::io_context> io_context, boost::asio::ssl::context context);

    void connect(const std::string &host, unsigned short port);
    bool verify_certificate(bool preverified, boost::asio::ssl::verify_context &ctx);
    void start();

    // has to be shared_ptr to ensure, that the context_thread (if detached) won't outlive the io_context, while still using it
    std::shared_ptr<boost::asio::io_context> io_context;
    boost::asio::ssl::context context;
    std::jthread context_thread;
};
