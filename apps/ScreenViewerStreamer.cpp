#include "streamer/ScreenViewerStreamer.hpp"
#include "streamer/X11IOController.hpp"

#include <spdlog/spdlog.h>

int main() {
    std::string email{"some_other_user@gmail.com"};
    std::string password{"superStrongPassword"};
    unsigned short proxy_server_port{44321};

    std::shared_ptr<ClientSocket> socket = std::make_shared<ClientSocket>("localhost", proxy_server_port, false);
    socket->login(email, password);
    auto id = socket->requestStreamerID();
    spdlog::info("Got id from server: '{}'. Starting streamer.", id);
    socket->waitForStartStreamMessage();

    auto io_controller = std::make_unique<X11IOController>();
    ScreenViewerStreamer streamer{std::move(socket), std::move(io_controller)};
    streamer.run();
    return 0;
}
