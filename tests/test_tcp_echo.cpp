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
#include "echo/tcp_server.hpp"

#include <gtest/gtest.h>

#include <arpa/inet.h>
using namespace net::service;
using namespace echo;

class TCPEchoServerTest : public ::testing::Test {};

TEST_F(TCPEchoServerTest, StartTest)
{
  using namespace io::socket;
  using server = basic_context_thread<tcp_server>;

  auto service = server();

  auto addr = socket_address<sockaddr_in>();
  addr->sin_family = AF_INET;
  addr->sin_port = htons(8080);

  service.start(addr);
  service.state.wait(async_context::PENDING);
  {
    using namespace io;
    auto sock = socket_handle(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr->sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_EQ(connect(sock, addr), 0);

    auto buf = std::array<char, 1>{'x'};
    auto msg = socket_message{.buffers = buf};

    const char *alphabet = "abcdefghijklmnopqrstuvwxyz";
    auto *end = alphabet + 26;

    for (auto *it = alphabet; it != end; ++it)
    {
      ASSERT_EQ(sendmsg(sock, socket_message{.buffers = std::span(it, 1)}, 0),
                1);
      ASSERT_EQ(recvmsg(sock, msg, 0), 1);
      EXPECT_EQ(buf[0], *it);
    }
  }
}

TEST_F(TCPEchoServerTest, ServerInitiatedSocketClose)
{
  using namespace io::socket;

  auto service = basic_context_thread<tcp_server>();

  auto addr = socket_address<sockaddr_in>();
  addr->sin_family = AF_INET;
  addr->sin_port = htons(8080);

  service.start(addr);
  service.state.wait(async_context::PENDING);
  {
    using namespace io;
    auto sock = socket_handle(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr->sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_EQ(connect(sock, addr), 0);

    service.signal(service.terminate);
    service.state.wait(async_context::STARTED);
  }
}
// NOLINTEND
