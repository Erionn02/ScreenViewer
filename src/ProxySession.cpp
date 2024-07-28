#include "ProxySession.hpp"
#include "ServerSessionsManager.hpp"

#include <spdlog/spdlog.h>


ProxySession::ProxySession(tcp::socket socket, asio::ssl::context &context,
                                                 std::weak_ptr<UsersManager> users_manager)
        : AuthenticatedServerSideClientSession(std::move(socket), context, std::move(users_manager)) {
}

void ProxySession::handleRead(BorrowedMessage message) {
    spdlog::info("[ProxySession] Got message! Type: {}, content: '{}'", MESSAGE_TYPE_TO_STR.at(message.type), message.content);
    switch (message.type) {
        case MessageType::REGISTER_STREAMER: {
            auto id = ServerSessionsManager::registerStreamer(std::static_pointer_cast<ProxySession>(shared_from_this()));
            spdlog::info("Registered streamer {}, id: {}", endpoint, id);
            send(BorrowedMessage{.type = MessageType::ID, .content = id});
            break;
        }
        case MessageType::FIND_STREAMER: {
            bool is_found = ServerSessionsManager::registerReceiver(std::static_pointer_cast<ProxySession>(shared_from_this()),
                                                    std::string{message.content});
            if(!is_found) {
                spdlog::info("Endpoint {} did not find streamer.", endpoint);
                sendNACK();
                scheduleNewAsyncRead();
            }
            break;
        }
        default: {
            send(OwnedMessage{.type = MessageType::JUST_A_MESSAGE, .content = fmt::format("Got your message! '{}'",
                                                                                          message.content)});
            scheduleNewAsyncRead();
        }

    }
}

