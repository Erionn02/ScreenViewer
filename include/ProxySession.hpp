#pragma once

#include "AuthenticatedSession.hpp"
#include "ScreenViewerBaseException.hpp"


#include <memory>

class ProxySessionException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


using namespace std::placeholders;
namespace asio = boost::asio;
using boost::asio::ip::tcp;
using boost::system::error_code;


class ProxySession: public AuthenticatedSession {
public:
    ProxySession(tcp::socket socket, boost::asio::ssl::context &context, std::weak_ptr<UsersManager> sessions_manager);
private:
    void handleRead(BorrowedMessage message) override;
};
