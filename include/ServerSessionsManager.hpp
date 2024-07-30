#pragma once

#include "ScreenViewerBaseException.hpp"


#include <memory>
#include <mutex>
#include <unordered_map>
#include <memory>
#include <thread>
#include <chrono>


class AuthenticatedSession;

class ServerSessionsManagerException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};

class ServerSessionsManager {
public:
    static void initCleanerThread(std::chrono::seconds client_timeout, std::chrono::seconds check_interval);

    static std::string registerStreamer(std::shared_ptr<AuthenticatedSession> streamer);
    static bool createBridgeWithStreamer(std::shared_ptr<AuthenticatedSession> receiver,
                                         const std::string &session_code);
    static std::size_t currentSessions();
    static void reset();
private:
    static std::string generateSessionID();
    static void terminateTimeoutClients(std::chrono::seconds client_timeout);

    struct TimedClientSession {
        std::shared_ptr<AuthenticatedSession> client_session;
        std::chrono::time_point<std::chrono::system_clock> time_point;
    };


    static inline std::mutex m{};
    static inline std::unordered_map<std::string, TimedClientSession> senders_sessions{};
    static inline std::jthread connections_controller{};
public:
    static constexpr std::size_t SESSION_ID_LENGTH{10};
    static constexpr std::chrono::seconds DEFAULT_CLIENT_TIMEOUT{120};
    static constexpr std::chrono::seconds DEFAULT_CHECK_INTERVAL{1};
};