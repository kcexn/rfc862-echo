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

auto tcp_service::operator()(async_context &ctx, const socket_dialog &socket,
                             const std::shared_ptr<read_context> &rctx,
                             std::span<const std::byte> buf) -> void
{
  service(ctx, socket, rctx, {.buffers = buf});
}

} // namespace echo
