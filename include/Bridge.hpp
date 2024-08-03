#pragma once

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <spdlog/spdlog.h>

#include <string>
#include <memory>


template<typename Stream_t>
class Bridge : public std::enable_shared_from_this<Bridge<Stream_t>> {
public:
    Bridge(Stream_t peer_one, Stream_t peer_two)
            : peer_one(std::move(peer_one)), peer_one_address(boost::lexical_cast<std::string>(this->peer_one.lowest_layer().remote_endpoint())),
              peer_two(std::move(peer_two)), peer_two_address(boost::lexical_cast<std::string>(this->peer_two.lowest_layer().remote_endpoint())) {}

    void start() {
        spdlog::info("Opening bridge [{} <-> {}]", peer_one_address, peer_two_address);
        boost::asio::async_read(peer_two,
                                boost::asio::buffer(client_buffer.data(), MAX_DATA_LENGTH),
                                boost::asio::transfer_at_least(1),
                                boost::bind(&Bridge::handle_upstream_read,
                                            std::enable_shared_from_this<Bridge<Stream_t>>::shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
        boost::asio::async_read(peer_one,
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
            boost::asio::write(peer_one,
                               boost::asio::buffer(client_buffer.data(), bytes_transferred),
                               boost::asio::transfer_all(), ec);
            handle_downstream_write(ec);
        } else {
            close();
        }
    }

    void handle_downstream_write(const boost::system::error_code &error) {
        if (!error) {
            boost::asio::async_read(peer_two,
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
            boost::asio::write(peer_two,
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
            boost::asio::async_read(peer_one,
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
        std::unique_lock lock{close_mutex};
        if (peer_one.lowest_layer().is_open()) {
            spdlog::info("Closing bridge [{} <-> {}]", peer_one_address, peer_two_address);
            peer_one.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            peer_one.lowest_layer().close();
        }

        if (peer_two.lowest_layer().is_open()) {
            spdlog::info("Closing bridge [{} <-> {}]", peer_two_address, peer_one_address);
            peer_one.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both);
            peer_two.lowest_layer().close();
        }
    }

    Stream_t peer_one;
    std::string peer_one_address;
    Stream_t peer_two;
    std::string peer_two_address;

    static constexpr int MAX_DATA_LENGTH = 1'000'000;
    std::array<unsigned char, MAX_DATA_LENGTH> server_buffer{};
    std::array<unsigned char, MAX_DATA_LENGTH> client_buffer{};

    std::mutex close_mutex;
};

using TCPBridge = Bridge<boost::asio::ip::tcp::socket>;
using SSLBridge = Bridge<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>;


