#include "ServerSessionsManager.hpp"
#include "ServerSideClientSession.hpp"

#include <spdlog/spdlog.h>

#include <random>
#include <filesystem>


ServerSessionsManager::ServerSessionsManager() : ServerSessionsManager(DEFAULT_CLIENT_TIMEOUT,
                                                     DEFAULT_CHECK_INTERVAL) {}

ServerSessionsManager::ServerSessionsManager(std::chrono::seconds client_timeout,
                                 std::chrono::seconds check_interval) {
    connections_controller = std::jthread{[client_timeout, check_interval, this](const std::stop_token &stop_token) {
        while (!stop_token.stop_requested()) {
            terminateTimeoutClients(client_timeout);
            std::this_thread::sleep_for(check_interval);
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

std::string ServerSessionsManager::registerStreamer(std::shared_ptr<ServerSideClientSession> sender, nlohmann::json json) {
    while (true) {
        auto session_id = generateSessionID();
        std::unique_lock lock{m};
        if (senders_sessions.contains(session_id)) {
            continue;
        }
        senders_sessions[session_id] = {.client_session = std::move(sender), .session_data = std::move(json),
                .time_point = std::chrono::high_resolution_clock::now()};
        return session_id;
    }
}

std::pair<std::shared_ptr<ServerSideClientSession>, nlohmann::json>
ServerSessionsManager::getStreamer(const std::string &session_code) {
    std::unique_lock lock{m};
    auto it = senders_sessions.find(session_code);
    if (it == senders_sessions.end()) {
        throw ServerSessionsManagerException{"Unknown session code."};
    }
    auto node = senders_sessions.extract(it);
    return {std::move(node.mapped().client_session), std::move(node.mapped().session_data)};
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
