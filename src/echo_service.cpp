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
 * @file echo_service.cpp
 * @brief This file defines the segment service.
 */
#include "echo/echo_service.hpp"

#include <spdlog/spdlog.h>

#include <charconv>
#include <string_view>

#include <arpa/inet.h>
namespace echo {

auto tcp_service::initialize(const socket_handle &sock) noexcept
    -> std::error_code
{
  return {};
}

auto tcp_service::service(async_context &ctx, const socket_dialog &socket,
                          const std::shared_ptr<read_context> &rctx,
                          const socket_message &msg) -> void
{
  using namespace stdexec;

  sender auto sendmsg =
      io::sendmsg(socket, msg, 0) |
      then([&, socket, rctx, bufs = msg.buffers](auto &&len) mutable {
        if (bufs += len; bufs)
          // NOLINTNEXTLINE(readability-avoid-return-with-void-value)
          return service(ctx, socket, rctx, {.buffers = bufs});

        reader(ctx, socket, rctx);
      }) |
      upon_error([](auto &&error) {}); // GCOVR_EXCL_LINE

  ctx.scope.spawn(std::move(sendmsg));
}

static auto getpeername(const tcp_service::socket_dialog &socket,
                        std::span<char> buf) -> std::string_view
{
  using namespace io::socket;
  std::memset(buf.data(), 0, buf.size());

  auto addr = socket_address<sockaddr_in6>();
  auto span = io::getpeername(socket, addr);
  const char *address =
      inet_ntop(addr->sin6_family, span.data(), buf.data(), span.size());
  auto len = std::strlen(address);

  unsigned short port = ntohs(addr->sin6_port);

  buf[len++] = ':';
  auto [ptr, err] =
      std::to_chars(buf.data() + len, buf.data() + buf.size(), port);
  if (err != std::errc{})
    return {};

  return {address};
}

auto tcp_service::operator()(async_context &ctx, const socket_dialog &socket,
                             const std::shared_ptr<read_context> &rctx,
                             std::span<const std::byte> buf) -> void
{
  using namespace io::socket;
  static constexpr auto BUFSIZE = 6UL;
  static auto BUF = std::array<char, INET6_ADDRSTRLEN + BUFSIZE>{};

  auto sockfd = static_cast<native_socket_type>(*socket.socket);
  if (rctx && !active_.contains(sockfd))
  {
    active_.emplace(sockfd);
    spdlog::info("New TCP connection from {}.", getpeername(socket, BUF));
  }

  if (!rctx && active_.contains(sockfd))
  {
    active_.erase(sockfd);
    spdlog::info("End TCP connection from {}.", getpeername(socket, BUF));
  }

  service(ctx, socket, rctx, {.buffers = buf});
}

} // namespace echo
