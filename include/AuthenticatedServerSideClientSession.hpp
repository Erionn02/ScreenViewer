#pragma once

#include "SocketBase.hpp"
#include "UsersManager.hpp"
#include "ScreenViewerBaseException.hpp"


#include <memory>

class ServerSideClientSessionException: public ScreenViewerBaseException {
public:
    using ScreenViewerBaseException::ScreenViewerBaseException;
};


using namespace std::placeholders;
namespace asio = boost::asio;
using boost::asio::ip::tcp;
using boost::system::error_code;


class AuthenticatedServerSideClientSession: public SocketBase {
public:
    AuthenticatedServerSideClientSession(tcp::socket socket, boost::asio::ssl::context &context, std::weak_ptr<UsersManager> users_manager);
    ~AuthenticatedServerSideClientSession() override;
    void start();
private:
    using PMF =  void (AuthenticatedServerSideClientSession::*)(BorrowedMessage);
    MessageHandler callback(PMF pmf);
    virtual void handleRead(BorrowedMessage message);
    void authenticateCallback(BorrowedMessage message);
    bool authenticate(BorrowedMessage message) const;

protected:
    void scheduleNewAsyncRead();

    std::weak_ptr<UsersManager> users_manager;
    std::string endpoint;
};
