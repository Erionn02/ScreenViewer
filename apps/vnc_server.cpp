#include "vnc/VNCServer.hpp"

int main() {
    std::uint32_t fps{30};
    VNCServer server{fps};

    server.run();

    return 0;
}
