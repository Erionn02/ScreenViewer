#pragma once

#include "SocketBase.hpp"

#include <pqxx/transaction>

#include <random>

class DummyTestSessionManager;

class SocketBaseWrapper : public SocketBase {
public:
    SocketBaseWrapper(tcp::socket socket, boost::asio::ssl::context &context,
                      std::weak_ptr<DummyTestSessionManager> test_session_manager) : SocketBase(
            {std::move(socket), context}), test_session_manager(std::move(test_session_manager)) {}

    void start();


    std::weak_ptr<DummyTestSessionManager> test_session_manager;
};

class DummyTestSessionManager {
public:
    DummyTestSessionManager(std::function<void(std::shared_ptr<SocketBase>)> set_test_socket) : set_test_socket(
            std::move(set_test_socket)) {}

    void setTestSocket(std::shared_ptr<SocketBase> socket) {
        set_test_socket(std::move(socket));
    }

    std::function<void(std::shared_ptr<SocketBase>)> set_test_socket;
};

inline void SocketBaseWrapper::start() {
    socket_.handshake(boost::asio::ssl::stream_base::server);
    if (auto manager = test_session_manager.lock()) {
        manager->setTestSocket(shared_from_this());
    }
}

inline std::string generateRandomString(std::size_t length) {
    static auto &chrs = "080886789"
                        "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    static thread_local std::mt19937 rg{std::random_device{}()};
    static thread_local std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string random_str;
    random_str.reserve(length);
    while (length--) {
        random_str += chrs[pick(rg)];
    }
    return random_str;
}

inline void clearDatabase(pqxx::connection& connection) {
    pqxx::work transaction{connection};
    transaction.exec(R"(DELETE FROM "user";)");
    transaction.commit();
}