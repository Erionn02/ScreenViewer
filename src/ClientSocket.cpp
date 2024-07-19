#include "ClientSocket.hpp"

#include <spdlog/spdlog.h>

ClientSocket::ClientSocket(const std::string &host, unsigned short port, bool verify_cert) : ClientSocket(
        std::make_unique<boost::asio::io_context>(),
        boost::asio::ssl::context{
                boost::asio::ssl::context::sslv23}) {
    if (verify_cert) {
        context.set_default_verify_paths();
        socket_.set_verify_mode(boost::asio::ssl::verify_peer);
        socket_.set_verify_callback([this](bool preverified, boost::asio::ssl::verify_context &ctx) {
            return verify_certificate(preverified, ctx);
        });
    } else {
        spdlog::warn("Skipping cert verification.");
        context.set_verify_mode(boost::asio::ssl::verify_fail_if_no_peer_cert);
        socket_.set_verify_callback([](bool, boost::asio::ssl::verify_context &) {
            return true;
        });
    }

    connect(host, port);
    start();
}

ClientSocket::ClientSocket(std::unique_ptr<boost::asio::io_context> io_context, boost::asio::ssl::context context)
        : SocketBase({*io_context, context}), io_context(std::move(io_context)), context(std::move(context)) {}

ClientSocket::~ClientSocket() {
    if (io_context) {
        io_context->stop();
        if (context_thread.joinable()) {
            context_thread.join();
        }
    }
}

void ClientSocket::connect(const std::string &host, unsigned short port) {
    spdlog::debug("Connecting to the endpoint: {}:{}", host, port);
    tcp::resolver resolver(socket_.get_executor());
    auto endpoints = resolver.resolve(host, std::to_string(port));
    if (endpoints.empty()) {
        throw SocketException(fmt::format("Did not find {}:{}", host, port));
    }
    socket_.lowest_layer().connect(*endpoints.begin());
    socket_.handshake(boost::asio::ssl::stream_base::client);
    spdlog::debug("Connected to the endpoint {}:{}.", host, port);
}

void ClientSocket::start() {
    context_thread = std::jthread{[io_context_ptr = io_context.get()] {
        io_context_ptr->run();
    }};
}

bool ClientSocket::verify_certificate(bool preverified, boost::asio::ssl::verify_context &ctx) {
    char subject_name[256];
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, sizeof(subject_name));
    spdlog::debug("Preverified {}, verifying {}", preverified, subject_name);
    return preverified;
}
