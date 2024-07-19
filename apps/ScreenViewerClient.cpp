#include "ClientSocket.hpp"

#include <iostream>



int main() {
    ClientSocket socket{"localhost", 2137u, false};

    std::string message_content{};
    do {
        std::cout << "Type a message to server: > ";
        std::getline(std::cin, message_content);
        socket.send(OwnedMessage{.type = MessageType::JUST_A_MESSAGE, .content = message_content});
        auto response = socket.receiveToBuffer();
        std::cout << "Server response: " << response.content << std::endl;
    } while(std::cin.good() && message_content != "Bye");


    return 0;
}