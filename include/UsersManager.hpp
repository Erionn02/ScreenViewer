#pragma once
#include "ScreenViewerBaseException.hpp"

#include <pqxx/connection>

#include <string>

class UsersManagerException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};

class UsersManager {
public:
    UsersManager(const std::string &database_address, const std::string &pg_user, const std::string &pg_password,
                 const std::string &database_name, unsigned short port = 5432);

    void addUser(const std::string& email, const std::string& password);
    bool authenticate(const std::string& email, const std::string& password);
private:
    void checkEmailConstraints(const std::string& email);
    void checkPasswordConstraints(const std::string& password);

    struct PreparedStatements {
        static constexpr const char* INSERT_USER{"INSERT_USER"};
        static constexpr const char* GET_PASSWORD_HASH{"GET_PASSWORD_HASH"};
    };

    pqxx::connection connection;
};