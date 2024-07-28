#include <gtest/gtest.h>

#include "Bridge.hpp"
#include "TestUtils.hpp"

#include <boost/asio.hpp>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

using boost::asio::ip::tcp;
using boost::system::error_code;

struct TCPBridgeTests : public testing::Test {
    const unsigned short TEST_PORT{48435};
    const std::string TEST_ADDRESS{"127.0.0.1"};
    asio::io_context io_context;
    ip::tcp::acceptor acceptor{io_context,
                               ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(TEST_ADDRESS), TEST_PORT)};


    std::future<std::shared_ptr<TCPBridge>> createBridge() {
        return std::async(std::launch::async, [&] {
            ip::tcp::socket downstream_socket{io_context};
            ip::tcp::socket upstream_socket{io_context};
            acceptor.accept(downstream_socket);
            acceptor.accept(upstream_socket);
            return std::make_shared<TCPBridge>(std::move(downstream_socket), std::move(upstream_socket));
        });

    }

    std::future<tcp::socket> connectToBridge() {
        return std::async(std::launch::async, [&] {
            tcp::socket socket{io_context};
            tcp::resolver resolver{io_context};
            connect(socket, resolver.resolve(TEST_ADDRESS, std::to_string(TEST_PORT)));
            return socket;
        });
    }

    std::pair<std::string, std::string>
    send(tcp::socket &receiver, tcp::socket &sender, std::size_t message_size) const {
        std::string response{};
        response.resize(message_size);

        auto random_str = generateRandomString(message_size);
        std::string message{random_str};

        write(receiver, asio::buffer(message));

        boost::asio::read(sender, boost::asio::buffer(response), boost::asio::transfer_exactly(random_str.size()));

        return {random_str, response};
    }

    void doTest(std::size_t message_size) {
        auto bridge_future = createBridge();

        auto first_client_future = connectToBridge();
        auto second_client_future = connectToBridge();

        std::jthread context_thread{[&](const std::stop_token &token) {
            while (!token.stop_requested()) {
                io_context.run();
                io_context.reset();
            }
        }};

        auto bridge = bridge_future.get();
        bridge->start();

        auto first_client = first_client_future.get();
        auto second_client = second_client_future.get();

        auto [first_message_sent, first_client_received_message] = send(first_client, second_client, message_size);
        auto [second_message_sent, second_client_received_message] = send(second_client, first_client, message_size);

        ASSERT_EQ(first_message_sent, first_client_received_message);
        ASSERT_EQ(second_message_sent, second_client_received_message);
    }
};


TEST_F(TCPBridgeTests, canSendSmallMessages) {
    std::size_t message_size{100};

    doTest(message_size);
}

TEST_F(TCPBridgeTests, canSendBigMessages) {
    std::size_t message_size{1'000'000};

    doTest(message_size);
}