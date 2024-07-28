#pragma once

#include "ScreenViewerBaseException.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <thread>
#include <chrono>


class ServerSideClientSession;

class ServerSessionsManagerException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};

class ServerSessionsManager {
public:
    ServerSessionsManager();
    ServerSessionsManager(std::chrono::seconds client_timeout, std::chrono::seconds check_interval);

    std::string registerStreamer(std::shared_ptr<ServerSideClientSession> sender,
                                 nlohmann::json json);
    std::pair<std::shared_ptr<ServerSideClientSession>, nlohmann::json> getStreamer(const std::string& session_code);

    std::size_t currentSessions();
private:
    std::string generateSessionID();
    void terminateTimeoutClients(std::chrono::seconds client_timeout);

    struct TimedClientSession {
        std::shared_ptr<ServerSideClientSession> client_session;
        nlohmann::json session_data;
        std::chrono::time_point<std::chrono::system_clock> time_point;
    };


    std::mutex m;
    std::unordered_map<std::string, TimedClientSession> senders_sessions;
    std::jthread connections_controller;
public:
    static constexpr std::size_t SESSION_ID_LENGTH{10};
    static constexpr std::chrono::seconds DEFAULT_CLIENT_TIMEOUT{120};
    static constexpr std::chrono::seconds DEFAULT_CHECK_INTERVAL{1};
};