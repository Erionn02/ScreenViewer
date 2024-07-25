#include <gtest/gtest.h>

#include "UsersManager.hpp"
#include <pqxx/transaction>

#include <fmt/format.h>


struct UsersManagerTests : public testing::Test {
    std::string database_address{"172.26.4.2"};
    std::string pg_user{"test"};
    std::string pg_password{"test"};
    std::string database_name{"screen-viewer"};

    pqxx::connection test_connection{fmt::format("dbname={} user={} password={} host={} port={}", database_name, pg_user,
                                                 pg_password, database_address, 5432)};
    UsersManager manager{database_address, pg_user, pg_password, database_name};

    std::string test_email{"some_user@gmail.com"};
    std::string test_password{"some_super_complicated_password_qwerty"};


    void SetUp() override {
        clearDatabase();
    }

    void TearDown() override {
        clearDatabase();
    }

    void clearDatabase() {
        pqxx::work transaction{test_connection};
        transaction.exec(R"(DELETE FROM "user";)");
        transaction.commit();
    }

    std::size_t getAmountOfUsers() {
        pqxx::work transaction{test_connection};
        auto result = transaction.exec(R"(SELECT COUNT(*) FROM "user";)");
        return result.at(0).at(0).as<std::size_t>();
    }
};



TEST_F(UsersManagerTests, canAddUser) {
    ASSERT_EQ(getAmountOfUsers(), 0);
    ASSERT_NO_THROW(manager.addUser(test_email, test_password));
    ASSERT_EQ(getAmountOfUsers(), 1);
}

TEST_F(UsersManagerTests, cannotAddTheSameUserTwice) {
    manager.addUser(test_email, test_password);
    ASSERT_THROW(manager.addUser(test_email, test_password), UsersManagerException);
    ASSERT_EQ(getAmountOfUsers(), 1);
}

TEST_F(UsersManagerTests, canAuthenticateUserWithCorrectCredentials) {
    manager.addUser(test_email, test_password);
    ASSERT_TRUE(manager.authenticate(test_email, test_password));
}

TEST_F(UsersManagerTests, cannotAuthenticateUserWithWrongPassword) {
    manager.addUser(test_email, test_password);
    ASSERT_FALSE(manager.authenticate(test_email, "some wrong password lol"));
}

TEST_F(UsersManagerTests, cannotAuthenticateUserThatDoesNotExist) {
    ASSERT_FALSE(manager.authenticate(test_email, test_password));
}