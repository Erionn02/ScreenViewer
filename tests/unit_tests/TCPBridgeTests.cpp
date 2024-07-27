#include <gtest/gtest.h>

#include "Bridge.hpp"

#include <boost/asio.hpp>
#include <iostream>
#include <thread>

namespace asio = boost::asio;
namespace ip = boost::asio::ip;

using boost::asio::ip::tcp;
using boost::system::error_code;

struct BridgeTests : public testing::Test {
    const unsigned short TEST_PORT{48435};
    const std::string TEST_ADDRESS{"127.0.0.1"};
    asio::io_context io_context;
    std::jthread j{[&]{io_context.run();}};
    ip::tcp::acceptor acceptor{io_context, ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(TEST_ADDRESS), TEST_PORT)};



    std::future<std::shared_ptr<Bridge<>>> createBridge() {
        return std::async(std::launch::async, [&]{
            ip::tcp::socket downstream_socket{io_context};
            ip::tcp::socket upstream_socket{io_context};
            acceptor.accept(downstream_socket);
            acceptor.accept(upstream_socket);
            return std::make_shared<Bridge<>>(std::move(downstream_socket), std::move(upstream_socket));
        });

    }

    template<std::size_t N>
    void scheduleAsyncRead(tcp::socket& socket, std::array<char, N>& reply) {
        socket.async_read_some(asio::buffer(reply), [&](error_code ec, std::size_t transferred_bytes) mutable {
            if (!ec) {
                std::cout<<"Server just wrote "<<std::string_view(reply.data(), transferred_bytes)<<std::endl;
                scheduleAsyncRead(socket, reply);
            } else {
                std::cout << "Encountered an error during async read, aborting. Details: " << ec.what() << std::endl;
            }
        });
    }

    std::future<tcp::socket> connectToBridge() {
        return std::async(std::launch::async, [&]{
            tcp::socket socket{io_context};
            tcp::resolver resolver{io_context};
            connect(socket, resolver.resolve(TEST_ADDRESS, std::to_string(TEST_PORT)));
            return socket;
        });
    }

};


TEST_F(BridgeTests, justWorks) {
    auto bridge_future = createBridge();

    auto first_client_future = connectToBridge();
    auto second_client_future = connectToBridge();

    auto bridge = bridge_future.get();
    bridge->start();

    auto first_client = first_client_future.get();
    auto second_client = second_client_future.get();
    std::array<char, 1024> buffer{};

    std::string message_content{"TEST_MESSAGE CONTENT"};
    write(first_client, asio::buffer(message_content), [](const boost::system::error_code &, const size_t &){});

    auto bytes_read = boost::asio::read(second_client, boost::asio::buffer(buffer), boost::asio::transfer_at_least(1));

    ASSERT_EQ(message_content.size(), bytes_read);
    ASSERT_EQ(message_content, (std::string_view{buffer.data(), bytes_read}));
}