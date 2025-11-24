/* Copyright (C) 2025 Kevin Exton (kevin.exton@pm.me)
 *
 * Echo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Echo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Echo.  If not, see <https://www.gnu.org/licenses/>.
 */
/**
 * @file tcp_server.hpp
 * @brief This file declares the TCP echo server.
 */
#pragma once
#ifndef ECHO_TCP_SERVER_HPP
#define ECHO_TCP_SERVER_HPP
#include <net/cppnet.hpp>

#include <chrono>
#include <optional>
/** @namespace For echo services. */
namespace echo {
/** @brief The service type to use. */
template <typename TCPStreamHandler>
using tcp_base = net::service::async_tcp_service<TCPStreamHandler, 0UL>;

/** @brief A TCP echo server. */
class tcp_server : public tcp_base<tcp_server> {
public:
  /** @brief The base class. */
  using Base = tcp_base<tcp_server>;
  /** @brief TCP buffer type. */
  using buffer_type = std::vector<std::byte>;
  /** @brief A connections type. */
  using connections = std::vector<std::optional<buffer_type>>;
  /** @brief The socket message type. */
  using socket_message = io::socket::socket_message<sockaddr_in6>;

  /**
   * @brief Constructs segment_service on the socket address.
   * @tparam T The type of the socket_address.
   * @param address The local IP address to bind to.
   */
  template <typename T>
  explicit tcp_server(socket_address<T> address) noexcept : Base(address)
  {}
  /**
   * @brief Initializes socket options.
   * @param sock The socket to initialize.
   * @returns A portable error_code.
   */
  [[nodiscard]] static auto
  initialize(const socket_handle &sock) noexcept -> std::error_code;

  /** @brief Runs when the server receives a terminate signal. */
  auto stop() noexcept -> void;

  /**
   * @brief Services the incoming socket_message.
   * @param ctx The asynchronous context of the message.
   * @param socket The socket that the message was read from.
   * @param rctx The read context that manages the read buffer lifetime.
   * @param msg The message that was read from the socket.
   */
  auto echo(async_context &ctx, const socket_dialog &socket,
            const std::shared_ptr<read_context> &rctx,
            const socket_message &msg) -> void;
  /**
   * @brief Receives the bytes emitted by the service_base reader.
   * @param ctx The asynchronous context of the message.
   * @param socket The socket that the message was read from.
   * @param rctx The read context that manages the read buffer lifetime.
   * @param buf The bytes that were read from the socket.
   */
  auto service(async_context &ctx, const socket_dialog &socket,
               const std::shared_ptr<read_context> &rctx,
               std::span<const std::byte> buf) -> void;

private:
  /** @brief The clock type. */
  using clock = std::chrono::steady_clock;
  /** @brief The timepoint type. */
  using time_point = clock::time_point;
  /** @brief The duration type. */
  using duration = std::chrono::milliseconds;
  /** @brief The drain timeout interval. */
  static constexpr auto DRAIN_TIMER = duration(5000);

  /** @brief Active connections. */
  connections active_;
  /** @brief Drain timeout. */
  std::optional<time_point> drain_timeout_;
};
} // namespace echo
#endif // ECHO_TCP_SERVER_HPP
