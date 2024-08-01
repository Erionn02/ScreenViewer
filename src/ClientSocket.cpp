#include "ClientSocket.hpp"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

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
        context_thread.request_stop();
        io_context->stop();
        if (context_thread.joinable()) {
            context_thread.join();
        }
    }
}

void ClientSocket::login(const std::string &email, const std::string &password) {
    nlohmann::json json;
    json["email"] = email;
    json["password"] = password;
    send(OwnedMessage{.type = MessageType::LOGIN, .content = to_string(json)});
    receiveACK();
}

bool ClientSocket::findOtherClient(const std::string &id) {
    send(BorrowedMessage {.type = MessageType::FIND_STREAMER, .content = id});
    auto message = receiveToBuffer();
    return message.type == MessageType::ACK;}

void ClientSocket::connect(const std::string &host, unsigned short port) {
    spdlog::debug("Connecting to the endpoint: {}:{}", host, port);
    tcp::resolver resolver(socket_.get_executor());
    auto endpoints = resolver.resolve(host, std::to_string(port));
    if (endpoints.empty()) {
        throw ScreenViewerBaseException(fmt::format("Did not find {}:{}", host, port));
    }
    socket_.lowest_layer().connect(*endpoints.begin());
    socket_.handshake(boost::asio::ssl::stream_base::client);
    spdlog::debug("Connected to the endpoint {}:{}.", host, port);
}

void ClientSocket::start() {
    context_thread = std::jthread{[io_context_ptr = io_context.get()](const std::stop_token& stop_token) {
        while(!stop_token.stop_requested()) {
            io_context_ptr->run();
            io_context_ptr->restart();
        }
    }};
}

bool ClientSocket::verify_certificate(bool preverified, boost::asio::ssl::verify_context &ctx) {
    char subject_name[256];
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
    X509_NAME_oneline(X509_get_subject_name(cert), subject_name, sizeof(subject_name));
    spdlog::debug("Preverified {}, verifying {}", preverified, subject_name);
    return preverified;
}

bool ClientSocket::waitForStartStreamMessage(std::chrono::seconds timeout) {
    spdlog::info("Waiting for client connection...");
    auto start = std::chrono::high_resolution_clock::now();
    while(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start) < timeout) {
        auto message = receiveToBuffer();
        if(message.type == MessageType::START_STREAM) {
            spdlog::info("Got connection, can start streaming now");
            return true;
        }
        spdlog::info("Got message while waiting: Type: {}", MESSAGE_TYPE_TO_STR.at(message.type));
    }
    return false;
}

std::string ClientSocket::requestStreamerID() {
    send(BorrowedMessage{.type=MessageType::REGISTER_STREAMER, .content{}});
    auto response = receive();
    if(response.type == MessageType::ID) {
        return response.content;
    }
    throw ScreenViewerBaseException(fmt::format("Could not register stream. Response type: {}", MESSAGE_TYPE_TO_STR.at(response.type)));
}

void ClientSocket::disconnect() {
    if(context_thread.joinable() && context_thread.get_stop_source().stop_possible()) {
        context_thread.get_stop_source().request_stop();
    }
    safeDisconnect(std::nullopt);
    if(context_thread.joinable()) {
        io_context->stop();
        context_thread.join();
    }
}
