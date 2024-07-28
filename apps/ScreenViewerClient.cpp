#include "ScreenViewerClient.hpp"

#include <iostream>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " stream_id" << std::endl;
        return 1;
    }
    std::string id{argv[1]};

    ClientSocket socket{"localhost", 44321, false};
    socket.login("some_user@gmail.com", "superStrongPassword");
    spdlog::info("Finding streamer with id: {}", id);
    bool is_found = socket.findStreamer(id);
    spdlog::info("Is streamer found: {}", is_found);

    if (is_found) {
        ScreenViewerClient client{std::move(socket)};
        client.run();
    }


    return 0;
}
