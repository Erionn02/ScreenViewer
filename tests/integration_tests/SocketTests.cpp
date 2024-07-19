#include <gtest/gtest.h>

#include "SocketBase.hpp"
#include "ClientSocket.hpp"
#include "ScreenViewerSessionsServer.hpp"

#include <random>


using namespace ::testing;

class DummyTestSessionManager;

class SocketBaseWrapper : public SocketBase {
public:
    SocketBaseWrapper(tcp::socket socket, boost::asio::ssl::context &context,
                      std::weak_ptr<DummyTestSessionManager> test_session_manager) : SocketBase(
            {std::move(socket), context}), test_session_manager(std::move(test_session_manager)) {}

    void start();


    std::weak_ptr<DummyTestSessionManager> test_session_manager;
};

class DummyTestSessionManager {
public:
    DummyTestSessionManager(std::function<void(std::shared_ptr<SocketBase>)> set_test_socket) : set_test_socket(
            std::move(set_test_socket)) {}

    void setTestSocket(std::shared_ptr<SocketBase> socket) {
        set_test_socket(std::move(socket));
    }

    std::function<void(std::shared_ptr<SocketBase>)> set_test_socket;
};

void SocketBaseWrapper::start() {
    socket_.handshake(boost::asio::ssl::stream_base::server);
    if (auto manager = test_session_manager.lock()) {
        manager->setTestSocket(shared_from_this());
    }
}

struct SocketTest : public Test {
    const unsigned short TEST_PORT{3421};
    std::shared_ptr<DummyTestSessionManager> test_session_manager{
            std::make_shared<DummyTestSessionManager>([this](auto socket) {
                setPeerSocket(std::move(socket));
            })};
    ScreenViewerSessionsServer<SocketBaseWrapper, DummyTestSessionManager> test_server{TEST_PORT,
                                                                                       CERTS_DIR,
                                                                                       test_session_manager};
    SocketBase::MessageHandler async_message_handler;
    std::jthread test_server_thread;
    std::atomic_flag is_peer_socket_set;
    std::shared_ptr<SocketBase> peer_socket{};

    void SetUp() override {
        spdlog::set_level(spdlog::level::debug);
        test_server_thread = std::jthread{[this] {
            test_server.run();
        }};
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override {
        test_server.stop();
    }

    void setPeerSocket(std::shared_ptr<SocketBase> other_peer_socker) {
        if (!is_peer_socket_set.test_and_set()) {
            peer_socket = std::move(other_peer_socker);
        }
    }

    void waitForPeerSocket() {
        while (!is_peer_socket_set.test()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::string generateRandomString(std::size_t length) {
        static auto &chrs = "080886789"
                            "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        static thread_local std::mt19937 rg{std::random_device{}()};
        static thread_local std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

        std::string random_str;
        random_str.reserve(length);
        while (length--) {
            random_str += chrs[pick(rg)];
        }
        return random_str;
    }

    ClientSocket createClientSocket() {
        ClientSocket client_socket{"localhost", TEST_PORT, false};
        waitForPeerSocket();
        return client_socket;
    }

    std::future<OwnedMessage> asyncReceiveMessages(std::promise<OwnedMessage> &response_promise, std::size_t how_many) {
        async_message_handler = [&, how_many](BorrowedMessage message) {
            static OwnedMessage final_received_message{.type = message.type};
            final_received_message.content += message.content;
            static std::size_t received_messages{0};
            if (++received_messages < how_many) {
                peer_socket->asyncReadMessage(async_message_handler);
            } else {
                response_promise.set_value(final_received_message);
            }
        };
        peer_socket->asyncReadMessage(async_message_handler);
        return response_promise.get_future();
    }
};


TEST_F(SocketTest, twoSocketsCanTalkToEachOther) {
    ClientSocket client_socket{"localhost", TEST_PORT, false};
    waitForPeerSocket();
    BorrowedMessage test_message{.type=MessageType::JUST_A_MESSAGE, .content = "Hello, world!"};
    client_socket.send(test_message);

    auto received_message = peer_socket->receiveToBuffer();

    ASSERT_EQ(test_message, received_message);
}

TEST_F(SocketTest, canReturnMessageCopy) {
    ClientSocket client_socket = createClientSocket();

    OwnedMessage test_message{.type=MessageType::JUST_A_MESSAGE, .content = "Hello, world!"};
    client_socket.send(test_message);


    auto received_message = peer_socket->receive();

    ASSERT_EQ(test_message, received_message);
}

TEST_F(SocketTest, returnsBufferPtr) {
    ClientSocket client_socket = createClientSocket();

    BorrowedMessage test_message{.type=MessageType::JUST_A_MESSAGE, .content = "Hello, world!"};
    client_socket.send(test_message);

    auto received_message = peer_socket->receiveToBuffer();

    std::string_view buffer_data = peer_socket->getBuffer();
    ASSERT_EQ(received_message.content, buffer_data.substr(0, received_message.content.size()));
}

TEST_F(SocketTest, canSendAndReceiveACK) {
    ClientSocket client_socket = createClientSocket();


    client_socket.sendACK();
    ASSERT_NO_THROW(peer_socket->receiveACK());
}

TEST_F(SocketTest, canReadMessageAsync) {
    ClientSocket client_socket = createClientSocket();


    std::size_t content_length{100'000};
    std::string content = generateRandomString(content_length);
    BorrowedMessage message{.type = MessageType::JUST_A_MESSAGE, .content = content};
    std::promise<BorrowedMessage> p{};
    peer_socket->asyncReadMessage([&](BorrowedMessage msg) {
        p.set_value(msg);
    });
    client_socket.send(message);


    ASSERT_EQ(p.get_future().get(), message);
}

TEST_F(SocketTest, canSendInPartsMessageBiggerThanBufferSize) {
    ClientSocket client_socket = createClientSocket();

    std::size_t compounded_messages{3};
    std::size_t message_size = compounded_messages * SocketBase::BUFFER_SIZE;
    std::string content = generateRandomString(message_size);
    OwnedMessage message{.type = MessageType::JUST_A_MESSAGE, .content = content};

    std::promise<OwnedMessage> result_promise;
    auto future_result = asyncReceiveMessages(result_promise, compounded_messages);
    client_socket.send(message);


    ASSERT_EQ(future_result.get(), message);
}

TEST_F(SocketTest, cannotAsynchronouslyReadMessageBiggerThanBufferSize) {
    ClientSocket client_socket = createClientSocket();

    ASSERT_THROW(client_socket.asyncReadMessage([](BorrowedMessage) {}, SocketBase::BUFFER_SIZE * 2), SocketException);
}
