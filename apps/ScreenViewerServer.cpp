#include "ScreenViewerSessionsServer.hpp"
#include "ServerSessionsManager.hpp"
#include "ServerSideClientSession.hpp"

#include <spdlog/spdlog.h>


int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Creating sessions manager...");
    auto sessions_manager = std::make_shared<ServerSessionsManager>();
    unsigned short port = 4321;
    spdlog::info("Starting server at port: {} with {} certs dir.", port, TEST_CERTS_DIR);
    ScreenViewerSessionsServer<ServerSideClientSession, ServerSessionsManager> server{port, TEST_CERTS_DIR, std::move(sessions_manager)};
    server.run();
}