#include <gtest/gtest.h>

#include "Servers.hpp"
#include "ClientSocket.hpp"
#include "TestUtils.hpp"
#include "ServerSessionsManager.hpp"

#include <filesystem>



using namespace ::testing;

struct ProxySessionTests : public Test {
    const unsigned short SERVER_TEST_PORT{41482};
    std::string database_address{"localhost"};
    std::string pg_user{"test"};
    std::string pg_password{"test"};
    std::string database_name{"screen-viewer"};
    std::string test_user_email_1{"some_email@gmail.com"};
    std::string test_user_email_2{"some_other_email@gmail.com"};
    std::string test_user_password{"wneoifwoefweg90234234mk234"};
    unsigned short database_test_port{54325};

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

    ClientSocket createClientSocket() {
        return {"localhost", SERVER_TEST_PORT, false};
    }

    void assertDisconnected(ClientSocket& socket) {
        try { // disconnect message if it gets through before actual disconnect
            auto response = socket.receiveToBuffer();
            ASSERT_EQ(response.type, MessageType::DISCONNECT);
        } catch(const boost::wrapexcept<boost::system::system_error>& e) {
            ASSERT_EQ(e.code().category().name(), std::string_view{"asio.ssl.stream"});
            ASSERT_EQ(e.code().value(), 1);
        }
    }

};

TEST_F(ProxySessionTests, doesNotLetUnauthenticatedUserSendAnyOtherThanLoginMessage) {
    auto client = createClientSocket();

    client.send(BorrowedMessage{.type = MessageType::JUST_A_MESSAGE,
            .content = "Some dummy data"});

    assertDisconnected(client);
}

TEST_F(ProxySessionTests, canLoginWithCorrectCredentials) {
    auto client = createClientSocket();



    users_manager->addUser(test_user_email_1, test_user_password);
    client.login(test_user_email_1, test_user_password);

    client.send(BorrowedMessage{.type = MessageType::JUST_A_MESSAGE, .content = "Some content"});
    auto response = client.receiveToBuffer();

    ASSERT_EQ(response.type, MessageType::RESPONSE);
    ASSERT_EQ(response.content, "Did not expect JUST_A_MESSAGE message.");
}

TEST_F(ProxySessionTests, canRequestStreamerID) {
    auto client = createClientSocket();



    users_manager->addUser(test_user_email_1, test_user_password);
    client.login(test_user_email_1, test_user_password);
    ASSERT_NO_THROW(client.requestStreamerID());
}

TEST_F(ProxySessionTests, cannotFindAnotherClientWithNonExistentID) {
    auto client = createClientSocket();


    users_manager->addUser(test_user_email_1, test_user_password);
    client.login(test_user_email_1, test_user_password);

    ASSERT_FALSE(client.findOtherClient("SomeNonExistentServerID"));
}

TEST_F(ProxySessionTests, twoClientsCanFindEachOther) {
    auto client_1 = createClientSocket();
    auto client_2 = createClientSocket();


    users_manager->addUser(test_user_email_1, test_user_password);
    users_manager->addUser(test_user_email_2, test_user_password);
    client_1.login(test_user_email_1, test_user_password);
    auto id = client_1.requestStreamerID();
    client_2.login(test_user_email_2, test_user_password);

    ASSERT_TRUE(client_2.findOtherClient(id));
}

TEST_F(ProxySessionTests, twoClientsCanTalkToEachOther) {
    auto client_1 = createClientSocket();
    auto client_2 = createClientSocket();


    users_manager->addUser(test_user_email_1, test_user_password);
    users_manager->addUser(test_user_email_2, test_user_password);
    client_1.login(test_user_email_1, test_user_password);
    auto id = client_1.requestStreamerID();
    client_2.login(test_user_email_2, test_user_password);
    client_2.findOtherClient(id);

    client_1.waitForStartStreamMessage();

    OwnedMessage msg{.type = MessageType::JUST_A_MESSAGE, .content = "Some content"};
    client_1.send(msg);
    auto received = client_2.receive();
    ASSERT_EQ(msg, received);
}