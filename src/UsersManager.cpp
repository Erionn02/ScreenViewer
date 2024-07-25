#include "UsersManager.hpp"

#include <fmt/format.h>
#include <bcrypt/BCrypt.hpp>
#include <pqxx/transaction>


UsersManager::UsersManager(const std::string &database_address, const std::string &pg_user,
                           const std::string &pg_password, const std::string &database_name) : connection(
        fmt::format("dbname={} user={} password={} host={} port={}", database_name, pg_user,
                    pg_password, database_address, 5432)) {
    connection.prepare(PreparedStatements::INSERT_USER, R"(INSERT INTO "user" VALUES($1, $2);)");
    connection.prepare(PreparedStatements::GET_PASSWORD_HASH, R"(SELECT password_hash FROM "user" WHERE email=$1;)");
}

void UsersManager::addUser(const std::string &email, const std::string &password) {
    checkEmailConstraints(email);
    checkPasswordConstraints(password);
    std::string hash = BCrypt::generateHash(password);
    pqxx::work transaction{connection};
    try {
        transaction.exec_prepared(PreparedStatements::INSERT_USER, email, hash);
        transaction.commit();
    } catch (const pqxx::unique_violation&) {
        throw UsersManagerException(fmt::format("User with '{}' email already exists.", email));
    }
}

bool UsersManager::authenticate(const std::string &email, const std::string &password) {
    pqxx::work transaction{connection};
    auto result = transaction.exec_prepared(PreparedStatements::GET_PASSWORD_HASH, email);
    if (result.empty()) {
        return false;
    }
    auto hash = result.at(0).at(0).as<std::string>();
    return BCrypt::validatePassword(password,hash);
}

void UsersManager::checkEmailConstraints(const std::string &) {
    // todo add constraints
}

void UsersManager::checkPasswordConstraints(const std::string &) {
    // todo add constraints
}
