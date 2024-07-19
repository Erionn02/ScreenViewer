#pragma once

#include "SocketBase.hpp"

#include <thread>
#include <fstream>

class ClientSocket : public SocketBase {
public:
    ClientSocket(const std::string &host, unsigned short port, bool verify_cert = true);
    ClientSocket(ClientSocket&&) = default;
    ~ClientSocket();

    void connect(const std::string &host, unsigned short port);
private:
    ClientSocket(std::unique_ptr<boost::asio::io_context> io_context, boost::asio::ssl::context context);

    bool verify_certificate(bool preverified, boost::asio::ssl::verify_context &ctx);
    void start();


    std::unique_ptr<boost::asio::io_context> io_context;
    boost::asio::ssl::context context;
    std::jthread context_thread;
};

// sadly io_context does not provide move constructor
// and I want to encapsulate io_context within the class
// therefore I use unique_ptr here to enable moving it
