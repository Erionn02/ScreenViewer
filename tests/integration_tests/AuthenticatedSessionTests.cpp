#include <gtest/gtest.h>

#include "Servers.hpp"
#include "ClientSocket.hpp"
#include "TestUtils.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>



using namespace ::testing;

struct AuthenticatedSessionTests : public Test {
    const unsigned short SERVER_TEST_PORT{41482};
    std::string database_address{"localhost"};
    std::string pg_user{"test"};
    std::string pg_password{"test"};
    std::string database_name{"screen-viewer"};
    unsigned short database_test_port{54325};

    pqxx::connection test_connection{fmt::format("dbname={} user={} password={} host={} port={}", database_name, pg_user,
                                                 pg_password, database_address, database_test_port)};
    std::shared_ptr<UsersManager> users_manager{std::make_shared<UsersManager>(database_address, pg_user, pg_password, database_name, database_test_port)};

    SessionsServer server{SERVER_TEST_PORT, TEST_DIR, users_manager};

    std::jthread server_thread;

    void SetUp() override {
        clearDatabase(test_connection);
        server_thread = std::jthread{[&]{
            server.run();
        }};
    }

    void TearDown () override {
        server.stop();
        server_thread.join();
        clearDatabase(test_connection);
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

TEST_F(AuthenticatedSessionTests, doesNotLetUnauthenticatedUserSendAnyOtherThanLoginMessage) {
    auto client = createClientSocket();

    client.send(BorrowedMessage{.type = MessageType::JUST_A_MESSAGE,
                                .content = "Some dummy data"});

    assertDisconnected(client);
}

TEST_F(AuthenticatedSessionTests, cannotLoginWithNotJsonData) {
    auto client = createClientSocket();
    
    client.send(BorrowedMessage{.type = MessageType::LOGIN,
            .content = "Something not-json-parsable"});

    assertDisconnected(client);
}


TEST_F(AuthenticatedSessionTests, cannotLoginWithWrongJsonStructure) {
    auto client = createClientSocket();

    nlohmann::json credentials{};
    credentials["erherh"] = "some unknown email";
    credentials["jtkjty"] = "some unknown password";

    client.send(BorrowedMessage{.type = MessageType::LOGIN,
            .content = to_string(credentials)});

    assertDisconnected(client);
}

TEST_F(AuthenticatedSessionTests, cannotLoginWithUnknownCredentials) {
    auto client = createClientSocket();

    ASSERT_ANY_THROW(client.login("some unknown email", "some unknown password"));
}

TEST_F(AuthenticatedSessionTests, canLoginWithCorrectCredentials) {
    auto client = createClientSocket();


    std::string test_user_email{"some_email@gmail.com"};
    std::string test_user_password{"wneoifwoefweg90234234mk234"};

    users_manager->addUser(test_user_email, test_user_password);

    ASSERT_NO_THROW(client.login(test_user_email, test_user_password));
}
