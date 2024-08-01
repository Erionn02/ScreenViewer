#include <gtest/gtest.h>

#include "Bridge.hpp"
#include "SocketBase.hpp"
#include "ClientSocket.hpp"
#include "ScreenViewerSessionsServer.hpp"
#include "TestUtils.hpp"

#include <atomic>


using namespace ::testing;


struct SSLBridgeTest : public Test {
    const unsigned short TEST_PORT{3421};
    std::shared_ptr<DummyTestSessionManager> test_session_manager{
            std::make_shared<DummyTestSessionManager>([this](auto socket) {
                static std::size_t i{0};
                if (i % 2 == 0) {
                    peer_socket = std::move(socket);
                } else {
                    bridge_atomic.store(std::make_shared<SSLBridge>(std::move(peer_socket->getSocket()),
                                                                    std::move(socket->getSocket())));
                    peer_socket.reset();
                }
                ++i;
            })};
    ScreenViewerSessionsServer<SocketBaseWrapper, DummyTestSessionManager> test_server{TEST_PORT,
                                                                                       TEST_DIR,
                                                                                       test_session_manager};
    std::jthread test_server_thread;
    std::shared_ptr<SocketBase> peer_socket{};
    std::atomic<std::shared_ptr<SSLBridge>> bridge_atomic;

    void SetUp() override {
        spdlog::set_level(spdlog::level::debug);
        test_server_thread = std::jthread{[&](const std::stop_token &token) {
            while (!token.stop_requested()) {
                spdlog::info("Run");
                test_server.run();
            }
        }};
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        test_server.stop();
    }


    std::shared_ptr<SSLBridge> waitForBridge() {
        std::shared_ptr<SSLBridge> bridge;
        while (!(bridge = bridge_atomic.load())) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return bridge;
    }

    void doTest(std::size_t content_size) {
        ClientSocket client1{"localhost", TEST_PORT, false};
        ClientSocket client2{"localhost", TEST_PORT, false};
        auto bridge = waitForBridge();
        bridge->start();


        auto content = generateRandomString(content_size);
        BorrowedMessage test_message{.type=MessageType::SCREEN_UPDATE, .content = content};
        client1.send(test_message);
        auto client2_received_message = client2.receiveToBuffer();

        test_message.content = content;
        client2.send(test_message);
        test_message.content = content;
        auto client1_received_message = client1.receiveToBuffer();
        ASSERT_EQ(test_message, client2_received_message);
        ASSERT_EQ(test_message, client1_received_message);
    }
};


TEST_F(SSLBridgeTest, twoSocketsCanTalkToEachOther) {
    std::size_t content_size{1'000};
    doTest(content_size);
}

TEST_F(SSLBridgeTest, twoSocketsCanTalkToEachOtherWithBigMessages) {
    std::size_t content_size{1'000'000};
    doTest(content_size);
}

