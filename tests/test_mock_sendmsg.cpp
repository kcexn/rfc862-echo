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
#include "echo/echo_service.hpp"

#include <gtest/gtest.h>

#include <cassert>
#include <list>

#include <arpa/inet.h>
using namespace net::service;
using namespace echo;

static int error = static_cast<int>(std::errc::bad_file_descriptor);
static bool test_error = false;
ssize_t sendmsg(int __fd, const struct msghdr *__message, int flags)
{
  std::size_t len = std::min(__message->msg_iov[0].iov_len, 1UL);
  auto len_ = send(__fd, __message->msg_iov[0].iov_base, len, flags);

  if (test_error)
    errno = error;

  return (test_error) ? -1 : len_;
}

class EchoServiceTest : public ::testing::Test {};

TEST_F(EchoServiceTest, SendMsgTest)
{
  using namespace io::socket;

  auto list = std::list<async_service<tcp_service>>{};
  auto &service = list.emplace_back();

  std::mutex mtx;
  std::condition_variable cvar;
  auto addr = socket_address<sockaddr_in>();
  addr->sin_family = AF_INET;
  addr->sin_port = htons(8080);

  service.start(mtx, cvar, addr);
  {
    auto lock = std::unique_lock{mtx};
    cvar.wait(lock, [&] { return service.interrupt || service.stopped; });
  }
  ASSERT_TRUE(static_cast<bool>(service.interrupt));
  {
    using namespace io;
    auto sock = socket_handle(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr->sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_EQ(connect(sock, addr), 0);

    auto buf = std::array<char, 2>{'x', 'y'};
    ASSERT_EQ(send(static_cast<int>(sock), buf.data(), 2, 0), 2);
  }
}

TEST_F(EchoServiceTest, TestError)
{
  using namespace io::socket;

  auto list = std::list<async_service<tcp_service>>{};
  auto &service = list.emplace_back();

  std::mutex mtx;
  std::condition_variable cvar;
  auto addr = socket_address<sockaddr_in>();
  addr->sin_family = AF_INET;
  addr->sin_port = htons(8080);

  service.start(mtx, cvar, addr);
  {
    auto lock = std::unique_lock{mtx};
    cvar.wait(lock, [&] { return service.interrupt || service.stopped; });
  }
  ASSERT_TRUE(static_cast<bool>(service.interrupt));
  {
    using namespace io;
    auto sock = socket_handle(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr->sin_addr.s_addr = inet_addr("127.0.0.1");

    ASSERT_EQ(connect(sock, addr), 0);

    auto buf = std::array<char, 2>{'x', 'y'};

    test_error = true;
    ASSERT_EQ(send(static_cast<int>(sock), buf.data(), 2, 0), 2);
  }
}
// NOLINTEND
