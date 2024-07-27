#pragma once
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <memory>

namespace ip = boost::asio::ip;

template<typename Socket_t = ip::tcp::socket>
class Bridge : public std::enable_shared_from_this<Bridge<Socket_t>> {
public:
    Bridge(Socket_t vnc_server_socket, Socket_t vnc_client_socket)
            : vnc_server_socket(std::move(vnc_server_socket)),
              vnc_client_socket(std::move(vnc_client_socket)) {}

    void start() {
        vnc_client_socket.async_read_some(
                boost::asio::buffer(client_buffer),
                boost::bind(&Bridge::handle_upstream_read,
                            std::enable_shared_from_this<Bridge<Socket_t>>::shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
        vnc_server_socket.async_read_some(
                boost::asio::buffer(server_buffer),
                boost::bind(&Bridge::handle_downstream_read,
                            std::enable_shared_from_this<Bridge<Socket_t>>::shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
    }

private:
    void handle_upstream_read(const boost::system::error_code &error,
                              const size_t &bytes_transferred) {
        if (!error) {
            async_write(vnc_server_socket,
                        boost::asio::buffer(client_buffer, bytes_transferred),
                        boost::bind(&Bridge::handle_downstream_write,
                                    std::enable_shared_from_this<Bridge<Socket_t>>::shared_from_this(),
                                    boost::asio::placeholders::error));
        } else {
            close();
        }
    }

    void handle_downstream_write(const boost::system::error_code &error) {
        if (!error) {
            vnc_client_socket.async_read_some(
                    boost::asio::buffer(client_buffer),
                    boost::bind(&Bridge::handle_upstream_read,
                                std::enable_shared_from_this<Bridge<Socket_t>>::shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
        } else {
            close();
        }
    }


    void handle_downstream_read(const boost::system::error_code &error,
                                const size_t &bytes_transferred) {
        if (!error) {
            async_write(vnc_client_socket,
                        boost::asio::buffer(server_buffer, bytes_transferred),
                        boost::bind(&Bridge::handle_upstream_write,
                                    std::enable_shared_from_this<Bridge<Socket_t>>::shared_from_this(),
                                    boost::asio::placeholders::error));
        } else {
            close();
        }
    }

    void handle_upstream_write(const boost::system::error_code &error) {
        if (!error) {
            vnc_server_socket.async_read_some(
                    boost::asio::buffer(server_buffer),
                    boost::bind(&Bridge::handle_downstream_read,
                                std::enable_shared_from_this<Bridge<Socket_t>>::shared_from_this(),
                                boost::asio::placeholders::error,
                                boost::asio::placeholders::bytes_transferred));
        } else {
            close();
        }
    }

    void close() {
        spdlog::info("Bridge closes.");
        std::unique_lock lock{close_mutex};

        if (vnc_server_socket.is_open()) {
            vnc_server_socket.close();
        }

        if (vnc_client_socket.is_open()) {
            vnc_client_socket.close();
        }
    }

    Socket_t vnc_server_socket;
    Socket_t vnc_client_socket;

    static constexpr int max_data_length = 8192;
    std::array<unsigned char, max_data_length> server_buffer{};
    std::array<unsigned char, max_data_length> client_buffer{};

    std::mutex close_mutex;
};


