#pragma once
#include <boost/asio/ssl/context_base.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>

using boost::asio::ip::tcp;

template<class CreatedSession_t, class UsersManager_t>
class ScreenViewerSessionsServer {
public:
    ScreenViewerSessionsServer(unsigned short port, const std::filesystem::path &key_cert_dir, std::shared_ptr<UsersManager_t> users_manager = std::make_shared<UsersManager_t>())
            : acceptor_(io_context, tcp::endpoint(boost::asio::ip::address(), port)),
              context_(boost::asio::ssl::context::sslv23),
              users_manager(std::move(users_manager)) {
        context_.set_options(
                boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2
                | boost::asio::ssl::context::verify_fail_if_no_peer_cert);
        context_.use_certificate_chain_file(key_cert_dir / "cert.pem");
        context_.use_private_key_file(key_cert_dir / "key.pem",
                                      boost::asio::ssl::context::pem);

        acceptNewConnection();
    }

    void run() {
        io_context.run();
    }

    void stop() {
        io_context.stop();
    }
private:
    void acceptNewConnection() {
        acceptor_.async_accept(
                [this](const boost::system::error_code &error, tcp::socket socket) {
                    if (!error) {
                        try {
                            auto endpoint = boost::lexical_cast<std::string>(socket.remote_endpoint());
                            spdlog::info("[ScreenViewerSessionsServer] Got new connection, endpoint: {}", endpoint);
                            std::make_shared<CreatedSession_t>(std::move(socket), context_,
                                                                      std::weak_ptr{users_manager})->start();
                        } catch(const std::exception& e) {
                            spdlog::error("[ScreenViewerSessionsServer] Encountered an unexpected exception while accepting new connection: {}", e.what());
                        }
                    }

                    acceptNewConnection();
                });
    }

    boost::asio::io_context io_context;
    tcp::acceptor acceptor_;
    boost::asio::ssl::context context_;
    std::shared_ptr<UsersManager_t> users_manager;
};
