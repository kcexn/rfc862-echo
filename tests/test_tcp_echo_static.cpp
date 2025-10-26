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
#ifndef ECHO_SERVER_STATIC_TEST
#define ECHO_SERVER_STATIC_TEST
#include "../src/echo_server.cpp"

#include <gtest/gtest.h>

#include <arpa/inet.h>
using namespace net::service;
using namespace echo;

class TCPEchoServerTest : public ::testing::Test {
  auto SetUp() -> void override
  {
    using namespace io::socket;

    auto addr_v4 = socket_address<sockaddr_in>();
    addr_v4->sin_family = AF_INET;
    addr_v4->sin_addr.s_addr = INADDR_ANY;
    addr_v4->sin_port = htons(8080);

    service_v4.start(mtx, cvar, addr_v4);
    {
      auto lock = std::unique_lock{mtx};
      cvar.wait(lock);
    }
    ASSERT_FALSE(static_cast<bool>(service_v4.stopped));

    auto addr_v6 = socket_address<sockaddr_in6>();
    addr_v6->sin6_family = AF_INET6;
    addr_v6->sin6_addr = IN6ADDR_ANY_INIT;
    addr_v6->sin6_port = htons(8081);

    service_v6.start(mtx, cvar, addr_v6);
    {
      auto lock = std::unique_lock{mtx};
      cvar.wait(lock);
    }
    ASSERT_FALSE(static_cast<bool>(service_v6.stopped));
  }

  auto TearDown() -> void override
  {
    service_v4.signal(service_v4.terminate);
    {
      auto lock = std::unique_lock{mtx};
      cvar.wait(lock, [&] { return service_v4.stopped.load(); });
    }

    service_v6.signal(service_v6.terminate);
    {
      auto lock = std::unique_lock{mtx};
      cvar.wait(lock, [&] { return service_v6.stopped.load(); });
    }
  }

  std::mutex mtx;
  std::condition_variable cvar;

protected:
  context_thread<tcp_server> service_v4;
  context_thread<tcp_server> service_v6;
};

TEST_F(TCPEchoServerTest, TestGetPeernameV4)
{
  using namespace io;
  using namespace io::socket;
  // Additional buffer length for the port number, the
  // square brackets, the colon, and the null byte.
  static constexpr auto BUFLEN = 9UL;
  auto sock = socket_handle(AF_INET, SOCK_STREAM, 0);

  auto addr = socket_address<sockaddr_in>();
  addr->sin_family = AF_INET;
  addr->sin_addr.s_addr = inet_addr("127.0.0.1");
  addr->sin_port = htons(8080);
  ASSERT_EQ(connect(sock, addr), 0);

  auto dialog = service_v4.poller.emplace(std::move(sock));

  auto buf = std::array<char, INET6_ADDRSTRLEN + BUFLEN>();
  auto address = getpeername_(dialog, buf);
  EXPECT_EQ(address, "127.0.0.1:8080");
}

TEST_F(TCPEchoServerTest, TestGetPeernameV6)
{
  using namespace io;
  using namespace io::socket;
  // Additional buffer length for the port number, the
  // square brackets, the colon, and the null byte.
  static constexpr auto BUFLEN = 9UL;
  auto sock = socket_handle(AF_INET6, SOCK_STREAM, 0);

  auto addr = socket_address<sockaddr_in6>();
  addr->sin6_family = AF_INET6;
  addr->sin6_addr = IN6ADDR_LOOPBACK_INIT;
  addr->sin6_port = htons(8081);

  ASSERT_EQ(connect(sock, addr), 0);

  auto dialog = service_v6.poller.emplace(std::move(sock));

  auto buf = std::array<char, INET6_ADDRSTRLEN + BUFLEN>();
  auto address = getpeername_(dialog, buf);
  EXPECT_EQ(address, "[::1]:8081");
}
#undef ECHO_SERVER_STATIC_TEST
#endif // ECHO_SERVER_STATIC_TEST
// NOLINTEND
