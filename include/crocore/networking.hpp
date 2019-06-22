// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

//  networking.h
//
//  Created by Fabian on 18/01/14.

#pragma once

#include "crocore.hpp"
#include "Connection.hpp"

namespace crocore::net {

extern char const *const UNKNOWN_IP;

std::string local_ip(bool ipV6 = false);

using tcp_connection_ptr = std::shared_ptr<class tcp_connection>;

void send_tcp(const std::string &str, const std::string &ip_string, uint16_t port);

void send_tcp(const std::vector<uint8_t> &bytes,
              const std::string &ip_string, uint16_t port);

void send_udp(const std::vector<uint8_t> &bytes,
              const std::string &ip_string, uint16_t port);

void send_udp_broadcast(const std::vector<uint8_t> &bytes, uint16_t port);

tcp_connection_ptr async_send_tcp(io_service_t &io_service,
                                  const std::string &str,
                                  const std::string &ip,
                                  uint16_t port);

tcp_connection_ptr async_send_tcp(io_service_t &io_service,
                                  const std::vector<uint8_t> &bytes,
                                  const std::string &the_ip,
                                  uint16_t the_port);

void async_send_udp(io_service_t &io_service,
                    const std::string &str,
                    const std::string &ip,
                    uint16_t port);

void async_send_udp(io_service_t &io_service,
                    const std::vector<uint8_t> &bytes,
                    const std::string &ip,
                    uint16_t port);

void async_send_udp_broadcast(io_service_t &io_service,
                              const std::vector<uint8_t> &bytes,
                              uint16_t port);

void async_send_udp_broadcast(io_service_t &io_service,
                              const std::string &str,
                              uint16_t port);

class udp_server
{
public:

    using receive_cb_t = std::function<void(const std::vector<uint8_t> &, const std::string &, uint16_t)>;

    explicit udp_server(io_service_t &io_service, receive_cb_t f = receive_cb_t());

    udp_server() = default;

    udp_server(udp_server &&the_other) noexcept;

    udp_server(const udp_server &) = delete;

    udp_server &operator=(udp_server the_other);

    void start_listen(uint16_t port);

    void stop_listen();

    void set_receive_function(receive_cb_t f = receive_cb_t());

    void set_receive_buffer_size(size_t sz);

    uint16_t listening_port() const;

private:
    std::shared_ptr<struct udp_server_impl> m_impl;
};

class tcp_server
{
public:

    using tcp_connection_callback = std::function<void(tcp_connection_ptr)>;

    explicit tcp_server(io_service_t &io_service, tcp_connection_callback ccb = tcp_connection_callback());

    tcp_server() = default;

    tcp_server(tcp_server &&other) noexcept;

    tcp_server(const tcp_server &) = delete;

    tcp_server &operator=(tcp_server other);

    void start_listen(uint16_t port);

    void stop_listen();

    void set_connection_callback(tcp_connection_callback ccb);

    uint16_t listening_port() const;

private:
    std::unique_ptr<struct tcp_server_impl> m_impl;
};

class tcp_connection : public Connection, public std::enable_shared_from_this<tcp_connection>
{
public:

    // tcp receive function
    using tcp_receive_cb_t = std::function<void(tcp_connection_ptr, const std::vector<uint8_t> &)>;

    static tcp_connection_ptr create(io_service_t &io_service,
                                     const std::string &ip,
                                     uint16_t port,
                                     tcp_receive_cb_t f = tcp_receive_cb_t());

    tcp_connection(const tcp_connection &) = delete;

    tcp_connection(tcp_connection &&) = delete;

    virtual ~tcp_connection();

    bool open() override;

    void close() override;

    bool is_open() const override;

    size_t read_bytes(void *buffer, size_t sz) override;

    size_t write_bytes(const void *buffer, size_t num_bytes) override;

    size_t available() const override;

    void drain() override;

    std::string description() const override;

    void set_receive_cb(receive_cb_t cb) override;

    void set_connect_cb(connection_cb_t cb) override;

    void set_disconnect_cb(connection_cb_t cb) override;

    void set_tcp_receive_cb(tcp_receive_cb_t f);

    uint16_t port() const;

    std::string remote_ip() const;

    uint16_t remote_port() const;

    double timeout() const;

    void set_timeout(double timeout_secs);

private:

    friend tcp_server_impl;
    std::shared_ptr<struct tcp_connection_impl> m_impl;

    tcp_connection(io_service_t &io_service, const std::string &ip, uint16_t port, tcp_receive_cb_t f);

    tcp_connection() = default;

    void start_receive();

    void check_deadline();
};

}// namespaces net kinski
