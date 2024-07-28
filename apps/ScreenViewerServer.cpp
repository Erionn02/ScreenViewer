#include "Servers.hpp"

#include <spdlog/spdlog.h>


int main() {
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Creating sessions manager...");
    std::string database_address{"localhost"};
    std::string pg_user{"postgres"};
    std::string pg_password{"root"};
    std::string database_name{"screen-viewer"};
    auto users_manager = std::make_shared<UsersManager>(database_address, pg_user, pg_password, database_name, 5432);
    const unsigned short SESSIONS_SERVER_PORT = 4321;
    const unsigned short PROXY_SERVER_PORT = 44321;
    spdlog::info("Starting server at SESSIONS_PORT: {} with {} certs dir.", SESSIONS_SERVER_PORT, TEST_CERTS_DIR);
    SessionsServer sessions_server{SESSIONS_SERVER_PORT, TEST_CERTS_DIR, users_manager};
    ProxyServer proxy_server{PROXY_SERVER_PORT, TEST_CERTS_DIR, users_manager};

    std::jthread t{[&]{
        proxy_server.run();
    }};
    sessions_server.run();
}