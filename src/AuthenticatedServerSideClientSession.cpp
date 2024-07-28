#include "AuthenticatedServerSideClientSession.hpp"

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>
#include <nlohmann/json.hpp>


AuthenticatedServerSideClientSession::AuthenticatedServerSideClientSession(tcp::socket socket, asio::ssl::context &context,
                                                                           std::weak_ptr<UsersManager> users_manager)
        : SocketBase({std::move(socket), context}), users_manager(std::move(users_manager)),
          endpoint(boost::lexical_cast<std::string>(socket_.next_layer().remote_endpoint())) {
}

AuthenticatedServerSideClientSession::~AuthenticatedServerSideClientSession() {
    spdlog::debug("AuthenticatedServerSideClientSession {} is being destroyed.", endpoint);
}

void AuthenticatedServerSideClientSession::start() {
    socket_.handshake(boost::asio::ssl::stream_base::server);
    constexpr std::size_t FIRST_MESSAGE_MAX_SIZE{1000};
    asyncReadMessage(callback(&AuthenticatedServerSideClientSession::authenticateCallback), FIRST_MESSAGE_MAX_SIZE);
}

SocketBase::MessageHandler AuthenticatedServerSideClientSession::callback(AuthenticatedServerSideClientSession::PMF pmf) {
    return [this, pmf](BorrowedMessage message) {
        std::invoke(pmf, this, message);
    };
}
void AuthenticatedServerSideClientSession::authenticateCallback(BorrowedMessage message) {
    if (message.type!=MessageType::LOGIN) {
        disconnect("You have to login first.");
        return;
    }
    if (authenticate(message)) {
        spdlog::info("Connection authenticated.");
        sendACK();
        scheduleNewAsyncRead();
    } else {
        spdlog::info("Failed to authenticate connection.");
        sendNACK();
    }
}

void AuthenticatedServerSideClientSession::scheduleNewAsyncRead() {
    asyncReadMessage(callback(&AuthenticatedServerSideClientSession::handleRead));
}

bool AuthenticatedServerSideClientSession::authenticate(BorrowedMessage message) const {
    spdlog::info("Authenticating connection...");
    auto content = nlohmann::json::parse(message.content);
    auto email = content.at("email").get<std::string>();
    auto password = content.at("password").get<std::string>();
    if (auto manager = users_manager.lock()) {
        return manager->authenticate(email, password);
    }
    return false;
}

void AuthenticatedServerSideClientSession::handleRead(BorrowedMessage message) {
    spdlog::info("[AuthenticatedServerSideClientSession] Got message! Type: {}, content: '{}'", static_cast<std::uint8_t>(message.type), message.content);

    send(OwnedMessage{.type = MessageType::JUST_A_MESSAGE, .content = fmt::format("Got your message! '{}'",
                                                                                  message.content)});

    scheduleNewAsyncRead();
}


