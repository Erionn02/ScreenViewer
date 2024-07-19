#include "ServerSideClientSession.hpp"

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>


ServerSideClientSession::ServerSideClientSession(tcp::socket socket, asio::ssl::context &context,
                                                 std::weak_ptr<SessionsManager> sessions_manager)
        : SocketBase({std::move(socket), context}), sessions_manager(std::move(sessions_manager)),
          endpoint(boost::lexical_cast<std::string>(socket_.next_layer().remote_endpoint())) {
}

ServerSideClientSession::~ServerSideClientSession() {
    spdlog::debug("ServerSideClientSession {} is being destroyed.", endpoint);
}

void ServerSideClientSession::start() {
    socket_.handshake(boost::asio::ssl::stream_base::server);
    asyncReadMessage(callback(&ServerSideClientSession::handleRead));
}

SocketBase::MessageHandler ServerSideClientSession::callback(ServerSideClientSession::PMF pmf) {
    return [this, pmf](BorrowedMessage message) {
        std::invoke(pmf, this, message);
    };
}

void ServerSideClientSession::handleRead(BorrowedMessage message) {
    spdlog::info("Got message! Type: {}, content: '{}'", static_cast<std::uint8_t>(message.type), message.content);

    send(OwnedMessage{.type = MessageType::JUST_A_MESSAGE, .content = fmt::format("Got your message! '{}'",
                                                                                  message.content)});

    asyncReadMessage(callback(&ServerSideClientSession::handleRead));
}

