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
 * @file tcp_server.cpp
 * @brief This file defines the TCP echo server.
 */
#include "echo/tcp_server.hpp"

#include <spdlog/spdlog.h>

#include <cassert>
#include <charconv>
#include <string_view>

#include <arpa/inet.h>
namespace echo {
// Additional buffer length for the port number, the square brackets,
// the colon, and the null byte.
static constexpr auto BUFLEN = 9UL;

// Dynamically configurable TCP buffer sizes for TCP sessions.
static constexpr auto TCP_BUFSIZE = 4 * 1024UL;

static inline auto
getpeername_(const tcp_server::socket_dialog &socket,
             std::span<char> buf) noexcept -> std::string_view
{
  assert(buf.size() >= INET6_ADDRSTRLEN + BUFLEN &&
         "Buffer must be large enough to print an IPv6 address and a port "
         "number.");

  using namespace io::socket;
  using io::getpeername;
  using std::to_chars;

  std::memset(buf.data(), 0, buf.size());
  unsigned short port = 0;
  std::size_t len = 0;

  auto addr = socket_address<sockaddr_in6>();
  auto span = getpeername(socket, addr);
  if (!span.data())
    return {};

  if (addr->sin6_family == AF_INET)
  {
    auto addr = socket_address<sockaddr_in>(span);
    inet_ntop(addr->sin_family, &addr->sin_addr, buf.data(), buf.size());
    port = ntohs(addr->sin_port);
    len = std::strlen(buf.data());
  }
  else
  {
    buf[0] = '[';
    inet_ntop(addr->sin6_family, &addr->sin6_addr, buf.data() + 1,
              buf.size() - 1);
    port = ntohs(addr->sin6_port);
    len = std::strlen(buf.data());
    buf[len++] = ']';
  }

  buf[len++] = ':';
  to_chars(buf.data() + len, buf.data() + buf.size(), port);

  return {buf.data()};
}

// Don't include the tcp_service method definitions if we are
// testing the static methods.
#ifndef ECHO_SERVER_STATIC_TEST
auto tcp_server::initialize(const socket_handle &sock) noexcept
    -> std::error_code
{
  return {};
}

auto tcp_server::stop() noexcept -> void
{
  using socket_type = io::socket::native_socket_type;

  if (drain_timeout_)
  {
    if (clock::now() >= *drain_timeout_)
    {
      spdlog::info("Stop requested. Closing TCP connections...");
      for (socket_type i = 0; i < static_cast<int>(active_.size()); ++i)
      {
        if (active_[i])
          shutdown(i, SHUT_RD);
      }
    }
  }
  else
  {
    spdlog::info("Stop requested. Draining TCP connections...");
    drain_timeout_ = clock::now() + DRAIN_TIMER;
  }
}

auto tcp_server::service(async_context &ctx, const socket_dialog &socket,
                         const std::shared_ptr<read_context> &rctx,
                         const socket_message &msg) -> void
{
  using namespace stdexec;
  if (!msg.buffers)
  {
    reader(ctx, socket, rctx);
    return;
  }

  sender auto sendmsg =
      io::sendmsg(socket, msg, MSG_NOSIGNAL) |
      then([&, socket, rctx, bufs = msg.buffers](auto &&len) mutable {
        if (bufs += len; bufs)
          // NOLINTNEXTLINE(readability-avoid-return-with-void-value)
          return service(ctx, socket, rctx, {.buffers = bufs});

        reader(ctx, socket, rctx);
      }) |
      upon_error([](auto &&error) {}); // GCOVR_EXCL_LINE

  ctx.scope.spawn(std::move(sendmsg));
}

auto tcp_server::operator()(async_context &ctx, const socket_dialog &socket,
                            const std::shared_ptr<read_context> &rctx,
                            std::span<const std::byte> buf) -> void
{
  using namespace io::socket;
  auto addrstr = std::array<char, INET6_ADDRSTRLEN + BUFLEN>();
  auto sockfd = static_cast<native_socket_type>(*socket.socket);

  if (active_.size() < static_cast<std::size_t>(sockfd) + 1)
  {
    active_.resize(sockfd + 1);
  }

  if (rctx && !active_[sockfd])
  {
    auto &bufptr = active_[sockfd] = buffer_type(TCP_BUFSIZE);
    rctx->msg.buffers = rctx->buffer = {*bufptr};
    spdlog::info("New TCP connection from {}.", getpeername_(socket, addrstr));
  }

  if (!rctx && active_[sockfd])
  {
    active_[sockfd].reset();
    spdlog::info("End TCP connection from {}.", getpeername_(socket, addrstr));
  }

  service(ctx, socket, rctx, {.buffers = buf});
}
#endif // ECHO_SERVER_STATIC_TEST

} // namespace echo
