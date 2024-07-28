#pragma once

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <memory>

#define DEBUG() spdlog::info(__FUNCTION__);


template<typename Stream_t>
class Bridge : public std::enable_shared_from_this<Bridge<Stream_t>> {
public:
    Bridge(Stream_t vnc_server_socket, Stream_t vnc_client_socket)
            : vnc_server_socket(std::move(vnc_server_socket)),
              vnc_client_socket(std::move(vnc_client_socket)) {}

    ~Bridge() {
        try {
            spdlog::info("Destroying bridge [{} <-> {}]",
                         boost::lexical_cast<std::string>(vnc_server_socket.next_layer().remote_endpoint()),
                         boost::lexical_cast<std::string>(vnc_client_socket.next_layer().remote_endpoint()));
        } catch (const std::exception& e) {
            spdlog::info("Destroying bridge, [{}]", e.what());
        }
    }

    void start() {
        boost::asio::async_read(vnc_client_socket,
                                boost::asio::buffer(client_buffer.data(), MAX_DATA_LENGTH),
                                boost::asio::transfer_at_least(1),
                                boost::bind(&Bridge::handle_upstream_read,
                                            std::enable_shared_from_this<Bridge<Stream_t>>::shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
        boost::asio::async_read(vnc_server_socket,
                                boost::asio::buffer(server_buffer.data(), MAX_DATA_LENGTH),
                                boost::asio::transfer_at_least(1),
                                boost::bind(&Bridge::handle_downstream_read,
                                            std::enable_shared_from_this<Bridge<Stream_t>>::shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

private:
    void handle_upstream_read(const boost::system::error_code &error,
                              const size_t &bytes_transferred) {
        if (!error) {
            boost::system::error_code ec;
            boost::asio::write(vnc_server_socket,
                               boost::asio::buffer(client_buffer.data(), bytes_transferred),
                               boost::asio::transfer_all(), ec);
            handle_downstream_write(ec);
        } else {
            close();
        }
    }

    void handle_downstream_write(const boost::system::error_code &error) {
        if (!error) {
            boost::asio::async_read(vnc_client_socket,
                                    boost::asio::buffer(client_buffer.data(), MAX_DATA_LENGTH),
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&Bridge::handle_upstream_read,
                                                std::enable_shared_from_this<Bridge<Stream_t>>::shared_from_this(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        } else {
            close();
        }
    }


    void handle_downstream_read(const boost::system::error_code &error,
                                const size_t &bytes_transferred) {
        if (!error) {
            boost::system::error_code ec;
            boost::asio::write(vnc_client_socket,
                               boost::asio::buffer(server_buffer.data(), bytes_transferred),
                               boost::asio::transfer_all(),
                               ec
            );
            handle_upstream_write(ec);
        } else {
            close();
        }
    }

    void handle_upstream_write(const boost::system::error_code &error) {
        if (!error) {
            boost::asio::async_read(vnc_server_socket,
                                    boost::asio::buffer(server_buffer.data(), MAX_DATA_LENGTH),
                                    boost::asio::transfer_at_least(1),
                                    boost::bind(&Bridge::handle_downstream_read,
                                                std::enable_shared_from_this<Bridge<Stream_t>>::shared_from_this(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        } else {
            close();
        }
    }

    void close() {
        spdlog::info("Bridge closes.");
        std::unique_lock lock{close_mutex};

        if (vnc_server_socket.lowest_layer().is_open()) {
            vnc_server_socket.lowest_layer().close();
        }

        if (vnc_client_socket.lowest_layer().is_open()) {
            vnc_client_socket.lowest_layer().close();
        }
    }

    Stream_t vnc_server_socket;
    Stream_t vnc_client_socket;

    static constexpr int MAX_DATA_LENGTH = 8192;
    std::array<unsigned char, MAX_DATA_LENGTH> server_buffer{};
    std::array<unsigned char, MAX_DATA_LENGTH> client_buffer{};

    std::mutex close_mutex;
};

using TCPBridge = Bridge<boost::asio::ip::tcp::socket>;
using SSLBridge = Bridge<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>;


