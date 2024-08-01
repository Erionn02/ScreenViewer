#include <gtest/gtest.h>

#include "IOControllerMock.hpp"
#include "Servers.hpp"
#include "ClientSocket.hpp"
#include "TestUtils.hpp"
#include "ServerSessionsManager.hpp"
#include "streamer/ScreenViewerStreamer.hpp"
#include "VideoDecoder.hpp"

#include <filesystem>



using namespace ::testing;

struct ScreenViewerStreamerTests : public Test {
    const unsigned short SERVER_TEST_PORT{41482};
    std::string database_address{"localhost"};
    std::string pg_user{"test"};
    std::string pg_password{"test"};
    std::string database_name{"screen-viewer"};
    std::string test_user_email_1{"some_email@gmail.com"};
    std::string test_user_email_2{"some_other_email@gmail.com"};
    std::string test_user_password{"wneoifwoefweg90234234mk234"};
    unsigned short database_test_port{54325};
    cv::Mat test_screenshot{cv::imread(TEST_DIR"/test_screenshot.png", cv::IMREAD_UNCHANGED)};


    pqxx::connection test_connection{fmt::format("dbname={} user={} password={} host={} port={}", database_name, pg_user,
                                                 pg_password, database_address, database_test_port)};
    std::shared_ptr<UsersManager> users_manager{std::make_shared<UsersManager>(database_address, pg_user, pg_password, database_name, database_test_port)};

    ProxyServer server{SERVER_TEST_PORT, TEST_DIR, users_manager};

    std::jthread server_thread;

    void SetUp() override {
        ServerSessionsManager::reset();
        clearDatabase(test_connection);
        server_thread = std::jthread{[&]{
            server.run();
        }};
    }

    void TearDown () override {
        server.stop();
        server_thread.join();
        clearDatabase(test_connection);
        ServerSessionsManager::reset();
    }

    std::shared_ptr<ClientSocket> createClientSocket() {
        return std::make_shared<ClientSocket>("localhost", SERVER_TEST_PORT, false);
    }

    std::shared_ptr<ClientSocket> createClient(const std::string& email, const std::string& password) {
        users_manager->addUser(email, password);
        auto client = createClientSocket();
        client->login(email, password);
        return client;
    }

    template<typename Event_t>
    void sendEvents(ClientSocket& client, MessageType message_type, std::size_t events_count) {
        for (std::size_t i=0; i< events_count; ++i) {
            Event_t event{};
            client.send(message_type, event);
        }
    }
};

TEST_F(ScreenViewerStreamerTests, streamerProcessesUserInputsAndSendsEncodedImages) {
    // given
    std::size_t keyboard_events_count{5};
    std::size_t mouse_events_count{13};

    auto streamer_socket = createClient(test_user_email_1, test_user_password);
    auto client_socket = createClient(test_user_email_2, test_user_password);
    auto io_controller = std::make_unique<IOControllerMock>();
    EXPECT_CALL(*io_controller, captureScreenshot).Times(AtLeast(1)).WillRepeatedly(Return(test_screenshot));

    // when
    auto id = streamer_socket->requestStreamerID();
    client_socket->findOtherClient(id);
    streamer_socket->waitForStartStreamMessage();

    ScreenViewerStreamer streamer{streamer_socket, std::move(io_controller)};
    std::jthread t{[&]{
        streamer.run();
    }};

    sendEvents<KeyboardEventData>(*client_socket, MessageType::KEYBOARD_INPUT, keyboard_events_count);
    auto encoded_image = client_socket->receiveToBuffer();
    sendEvents<MouseEventData>(*client_socket, MessageType::MOUSE_INPUT, mouse_events_count);

    // then
    VideoDecoder decoder{};
    AVPacket packet{};
    packet.data = std::bit_cast<uint8_t *>(encoded_image.content.data());
    packet.size = static_cast<int>(encoded_image.content.size());
    auto decoded_image = decoder.decode(&packet);

    ASSERT_EQ(decoded_image.rows, test_screenshot.rows);
    ASSERT_EQ(decoded_image.cols, test_screenshot.cols);
    ASSERT_EQ(decoded_image.channels(), 3);

    client_socket->disconnect();
    streamer_socket->disconnect();
    t.join();
}
