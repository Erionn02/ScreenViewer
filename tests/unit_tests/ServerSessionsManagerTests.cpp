#include <gtest/gtest.h>

#include "ServerSessionsManager.hpp"

struct ServerSessionsManagerTests : public testing::Test {

    void SetUp() override {
        ServerSessionsManager::reset();
    }

    void TearDown() override {
        ServerSessionsManager::reset();
    }
};

TEST_F(ServerSessionsManagerTests, canRegisterStreamer) {
    auto id = ServerSessionsManager::registerStreamer(nullptr);
    ASSERT_NE(id, "");
    ASSERT_EQ(ServerSessionsManager::currentSessions(), 1);
}

TEST_F(ServerSessionsManagerTests, resetRemovesSessions) {
    ServerSessionsManager::registerStreamer(nullptr);
    ASSERT_EQ(ServerSessionsManager::currentSessions(), 1);
    ServerSessionsManager::reset();
    ASSERT_EQ(ServerSessionsManager::currentSessions(), 0);
}

TEST_F(ServerSessionsManagerTests, removesTimeoutedClient) {
    std::chrono::seconds client_timeout{2};
    std::chrono::seconds check_interval{0};
    ServerSessionsManager::initCleanerThread(client_timeout, check_interval);


    ServerSessionsManager::registerStreamer(nullptr);
    ASSERT_EQ(ServerSessionsManager::currentSessions(), 1);
    std::this_thread::sleep_for(client_timeout);
    ASSERT_EQ(ServerSessionsManager::currentSessions(), 0);
}