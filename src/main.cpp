#include "echo/detail/argument_parser.hpp"
#include "echo/echo_service.hpp"

#include <charconv>
#include <condition_variable>
#include <csignal>
#include <filesystem>
#include <format>
#include <iostream>
#include <thread>

using namespace net::service;
using namespace echo;

using service_type = async_service<tcp_service>;

static constexpr unsigned short PORT = 7;
static constexpr char const *const usage = "usage: {} [<port>]\n";

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

  return {};
}

struct config {
  unsigned short port = PORT;
};

auto parse_args(int argc, char const *const *argv) -> std::optional<config>
{
  using namespace echo::detail;

  auto conf = config();
  char const *const progname = std::filesystem::path(*argv).stem().c_str();
  for (const auto &option : argument_parser::parse(argc, argv))
  {
    if (!option.flag.empty())
    {
      if (option.flag == "-h" || option.flag == "--help")
      {
        std::cout << std::format(usage, progname);
      }
      else
      {
        std::cerr << std::format("Unrecognized flag: {}\n", option.flag)
                  << std::format(usage, progname);
      }
      return std::nullopt;
    }

    auto [ptr, err] =
        std::from_chars(option.value.cbegin(), option.value.cend(), conf.port);
    if (err != std::errc{})
    {
      std::cerr << std::format("Invalid port number: {}\n", option.value)
                << std::format(usage, progname);
      return std::nullopt;
    }
  }

  return {conf};
}

auto main(int argc, char *argv[]) -> int
{
  using namespace io::socket;

  if (auto conf = parse_args(argc, argv))
  {
    auto mtx = std::mutex{};
    auto cvar = std::condition_variable{};

    auto address = socket_address<sockaddr_in>{};
    address->sin_family = AF_INET;
    address->sin_port = htons(conf->port);

    auto service = service_type{};
    auto sighandler = signal_handler(service);

    service.start(mtx, cvar, address);

    auto lock = std::unique_lock{mtx};
    cvar.wait(lock, [&] { return service.stopped.load(); });
  }

  return 0;
}
