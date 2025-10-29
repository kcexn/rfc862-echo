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
 * @file udp_server.cpp
 * @brief This file defines the UDP echo server.
 */
#include "echo/udp_server.hpp"

namespace echo {
[[nodiscard]] auto
udp_server::initialize(const socket_handle &sock) noexcept -> std::error_code
{
  return {};
}

auto udp_server::service(async_context &ctx, const socket_dialog &socket,
                         const std::shared_ptr<read_context> &rctx,
                         const socket_message &msg) -> void
{
  using namespace stdexec;
  sender auto sendmsg = io::sendmsg(socket, msg, MSG_NOSIGNAL) |
                        then([&, socket, rctx, msg](auto &&len) mutable {
                          reader(ctx, socket, rctx);
                        }) |
                        upon_error([](auto &&error) {}); // GCOVR_EXCL_LINE

  ctx.scope.spawn(std::move(sendmsg));
}
/**
 * @brief Receives the bytes emitted by the service_base reader.
 * @param ctx The asynchronous context of the message.
 * @param socket The socket that the message was read from.
 * @param rctx The read context that manages the read buffer lifetime.
 * @param buf The bytes that were read from the socket.
 */
auto udp_server::operator()(async_context &ctx, const socket_dialog &socket,
                            const std::shared_ptr<read_context> &rctx,
                            std::span<const std::byte> buf) -> void
{
  using namespace io::socket;
  if (!rctx)
    return;

  auto address = *rctx->msg.address;
  if (address->sin6_family == AF_INET)
  {
    const auto *ptr =
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<struct sockaddr *>(std::ranges::data(address));
    address = socket_address<sockaddr_in>(ptr);
  }
  service(ctx, socket, rctx, {.address = {address}, .buffers = buf});
}
} // namespace echo
