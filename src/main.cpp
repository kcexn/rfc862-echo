#include "echo/echo_service.hpp"

#include <charconv>
#include <condition_variable>
#include <csignal>
#include <iostream>
#include <thread>

using namespace net::service;
using namespace echo;

using service_type = async_service<tcp_service>;

static constexpr short PORT = 7;
static const char *const usage = "usage: echo [<port>]\n";

static auto signal_mask() -> sigset_t *
{
  static auto set = sigset_t{};
  static sigset_t *setp = nullptr;
  static auto mtx = std::mutex{};

  if (auto lock = std::lock_guard{mtx}; !setp)
  {
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGINT);
    setp = &set;
  }
  return setp;
}

static auto signal_handler(service_type &service) -> std::jthread
{
  static const sigset_t *sigmask = nullptr;
  static auto mtx = std::mutex();

  if (auto lock = std::lock_guard{mtx}; !sigmask)
  {
    sigmask = signal_mask();
    pthread_sigmask(SIG_BLOCK, sigmask, nullptr);
  }

  return std::jthread([&](const std::stop_token &token) noexcept {
    static const auto timeout = timespec{.tv_sec = 0, .tv_nsec = 50000000};

    while (!token.stop_requested())
    {
      using enum service_type::signals;
      switch (sigtimedwait(sigmask, nullptr, &timeout))
      {
        case SIGTERM:
        case SIGHUP:
        case SIGINT:
          service.signal(terminate);
          break;

        default:
          break;
      }
    }
  });
}

auto main(int argc, char *argv[]) -> int
{
  using namespace io::socket;

  short port = PORT;
  if (argc > 1)
  {
    // NOLINTBEGIN
    if (strncmp(argv[1], "-h", 2) == 0)
    {
      std::cout << usage;
      return 0;
    }

    auto size = std::strlen(argv[1]);
    auto [ptr, err] = std::from_chars(argv[1], argv[1] + size, port);
    if (err != std::errc{})
    {
      std::cerr << "Invalid port number: " << argv[1] << "\n" << usage;
      return 0;
    }
    // NOLINTEND
  }

  auto mtx = std::mutex{};
  auto cvar = std::condition_variable{};

  auto address = socket_address<sockaddr_in>{};
  address->sin_family = AF_INET;
  address->sin_port = htons(port);

  auto service = service_type{};
  auto sighandler = signal_handler(service);

  service.start(mtx, cvar, address);

  auto lock = std::unique_lock{mtx};
  cvar.wait(lock, [&] { return service.stopped.load(); });

  return 0;
}
