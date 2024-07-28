#pragma once

#include "SocketBase.hpp"
#include "ServerSessionsManager.hpp"
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


class ServerSideClientSession: public SocketBase {
public:
    ServerSideClientSession(tcp::socket socket, boost::asio::ssl::context &context, std::weak_ptr<ServerSessionsManager> sessions_manager);
    ~ServerSideClientSession();
    void start();
private:
    using PMF =  void (ServerSideClientSession::*)(BorrowedMessage);
    MessageHandler callback(PMF pmf);
    void handleRead(BorrowedMessage message);

    std::weak_ptr<ServerSessionsManager> sessions_manager;
    std::string endpoint;
};
