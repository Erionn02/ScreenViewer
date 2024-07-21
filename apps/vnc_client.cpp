#include "vnc/VNCClient.hpp"

#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr <<"Usage:"<<argv[0]<<"<server_ip>\n";
        return 1;
    }
    VNCClient client{argv[1]};

    client.run();


    return 0;
}
