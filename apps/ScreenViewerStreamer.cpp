#include "ScreenViewerStreamer.hpp"

#include <spdlog/spdlog.h>

int main() {
    std::string email{"some_other_user@gmail.com"};
    std::string password{"superStrongPassword"};
    unsigned short proxy_server_port{44321};

    ClientSocket socket{"localhost", proxy_server_port, false};
    socket.login(email, password);
    auto id = socket.requestStreamerID();
    spdlog::info("Got id from server: '{}'. Starting streamer.", id);
    socket.waitForStartStreamMessage();

    ScreenViewerStreamer streamer{std::move(socket)};
    streamer.run();
    return 0;
}
