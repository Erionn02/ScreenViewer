#include "AuthenticatedSession.hpp"

#include <spdlog/spdlog.h>
#include <boost/lexical_cast.hpp>
#include <nlohmann/json.hpp>


AuthenticatedSession::AuthenticatedSession(tcp::socket socket, asio::ssl::context &context,
                                           std::weak_ptr<UsersManager> users_manager)
        : SocketBase({std::move(socket), context}), users_manager(std::move(users_manager)),
          endpoint(boost::lexical_cast<std::string>(socket_.next_layer().remote_endpoint())) {
}

AuthenticatedSession::~AuthenticatedSession() {
    spdlog::debug("AuthenticatedSession {} is being destroyed.", endpoint);
}

void AuthenticatedSession::start() {
    socket_.handshake(boost::asio::ssl::stream_base::server);
    constexpr std::size_t FIRST_MESSAGE_MAX_SIZE{1000};
    asyncReadMessage(callback(&AuthenticatedSession::authenticateCallback), FIRST_MESSAGE_MAX_SIZE);
}

SocketBase::MessageHandler AuthenticatedSession::callback(AuthenticatedSession::PMF pmf) {
    return [this, pmf](BorrowedMessage message) {
        std::invoke(pmf, this, message);
    };
}
void AuthenticatedSession::authenticateCallback(BorrowedMessage message) {
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

void AuthenticatedSession::scheduleNewAsyncRead() {
    asyncReadMessage(callback(&AuthenticatedSession::handleRead));
}

bool AuthenticatedSession::authenticate(BorrowedMessage message) const {
    spdlog::info("Authenticating connection...");
    auto content = nlohmann::json::parse(message.content);
    auto email = content.at("email").get<std::string>();
    auto password = content.at("password").get<std::string>();
    if (auto manager = users_manager.lock()) {
        return manager->authenticate(email, password);
    }
    return false;
}

void AuthenticatedSession::handleRead(BorrowedMessage message) {
    spdlog::info("[AuthenticatedSession] Got message! Type: {}, content: '{}'", static_cast<std::uint8_t>(message.type), message.content);

    send(OwnedMessage{.type = MessageType::JUST_A_MESSAGE, .content = fmt::format("Got your message! '{}'",
                                                                                  message.content)});

    scheduleNewAsyncRead();
}


