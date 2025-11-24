/* Copyright (C) 2025 Kevin Exton (kevin.exton@pm.me)
 *
 * Cloudbus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cloudbus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Cloudbus.  If not, see <https://www.gnu.org/licenses/>.
 */

// NOLINTBEGIN
#include "echo/udp_server.hpp"

#include <gtest/gtest.h>

#include <arpa/inet.h>
using namespace net::service;
using namespace echo;

class UDPEchoServerTest : public ::testing::Test {};

TEST_F(UDPEchoServerTest, EchoTest)
{
  using namespace io::socket;

  auto service = basic_context_thread<udp_server>();

  auto addr = socket_address<sockaddr_in>();
  addr->sin_family = AF_INET;
  addr->sin_port = htons(8080);

  service.start(addr);
  service.state.wait(async_context::PENDING);
  {
    using namespace io;
    auto sock = socket_handle(AF_INET, SOCK_DGRAM, 0);
    addr->sin_addr.s_addr = inet_addr("127.0.0.1");

    auto buf = std::array<char, 1>{'x'};
    auto msg = socket_message<sockaddr_in>{
        .address = {socket_address<sockaddr_in>()}, .buffers = buf};

    const char *alphabet = "abcdefghijklmnopqrstuvwxyz";
    auto *end = alphabet + 26;

    for (auto *it = alphabet; it != end; ++it)
    {
      ASSERT_EQ(sendmsg(sock,
                        socket_message<sockaddr_in>{
                            .address = {addr}, .buffers = std::span(it, 1)},
                        0),
                1);
      ASSERT_EQ(recvmsg(sock, msg, 0), 1);
      EXPECT_EQ(*msg.address, addr);
      EXPECT_EQ(buf[0], *it);
    }
  }
}
// NOLINTEND
