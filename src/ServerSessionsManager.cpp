#include "ServerSessionsManager.hpp"
#include "AuthenticatedSession.hpp"
#include "Bridge.hpp"

#include <spdlog/spdlog.h>

#include <random>
#include <filesystem>
#include <condition_variable>


void
ServerSessionsManager::initCleanerThread(std::chrono::seconds client_timeout, std::chrono::seconds check_interval) {
    connections_controller = std::jthread{[client_timeout, check_interval](const std::stop_token &stop_token) {
        std::mutex cv_mutex;
        std::condition_variable cv;
        while (!stop_token.stop_requested()) {
            terminateTimeoutClients(client_timeout);
            std::unique_lock lock{cv_mutex};
            cv.wait_for(lock, check_interval, [&]{return stop_token.stop_requested();});
        }
    }};
}

void ServerSessionsManager::terminateTimeoutClients(std::chrono::seconds client_timeout) {
    std::unique_lock lock{m};
    for (auto it = senders_sessions.begin(); it != senders_sessions.end();) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed_time = (now - it->second.time_point);
        if (elapsed_time > client_timeout) {
            spdlog::info("Terminating sender client with code: '{}'", it->first);
            it = senders_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

std::string ServerSessionsManager::registerStreamer(std::shared_ptr<AuthenticatedSession> sender) {
    while (true) {
        auto session_id = generateSessionID();
        std::unique_lock lock{m};
        if (senders_sessions.contains(session_id)) {
            continue;
        }
        senders_sessions[session_id] = {.client_session = std::move(sender),
                .time_point = std::chrono::high_resolution_clock::now()};
        return session_id;
    }
}


bool ServerSessionsManager::createBridgeWithStreamer(std::shared_ptr<AuthenticatedSession> receiver,
                                                     const std::string & session_code) {

    std::unique_lock lock{m};
    bool is_streamer_found = senders_sessions.contains(session_code);
    if(is_streamer_found) {
        auto sender_node = senders_sessions.extract(session_code);
        lock.unlock();
        receiver->sendACK();
        auto& sender_session = sender_node.mapped().client_session;
        sender_session->send(BorrowedMessage {.type = MessageType::START_STREAM, .content{}});
        auto bridge = std::make_shared<SSLBridge>(std::move(sender_session->getSocket()), std::move(receiver->getSocket()));
        spdlog::info("SSLBridge created!");
        bridge->start();
        spdlog::info("SSLBridge started!");
    }
    return is_streamer_found;
}

std::string ServerSessionsManager::generateSessionID() {
    static auto &chrs = "080886789"
                        "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    static thread_local std::mt19937 rg{std::random_device{}()};
    static thread_local std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::size_t length{SESSION_ID_LENGTH};
    std::string random_str;
    random_str.reserve(length);
    while (length--) {
        random_str += chrs[pick(rg)];
    }
    return random_str;
}

std::size_t ServerSessionsManager::currentSessions() {
    std::unique_lock lock{m};
    return senders_sessions.size();
}

void ServerSessionsManager::reset() {
    std::unique_lock lock{m};
    senders_sessions.clear();
    if(connections_controller.joinable()) {
        connections_controller.request_stop();
        connections_controller.join();
    }
}
