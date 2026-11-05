#include "config.hpp"

#ifndef CONFIG_MESON_SYSTEM_LINUX
#  error "This executable should only be built for Linux targets."
#endif

#include <cinttypes>
#include <cstddef>

#include <iostream>
#include <format>
#include <stdexcept>
#include <array>

#include <signal.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <termios.h>
#include <poll.h>

#include <sys/signalfd.h>

#define POSIX_CHECK(exp) \
  do \
  { \
    if ((exp) < 0) \
    { \
      auto buffer = std::array<char, 1024>{ }; \
      const auto *const str = strerror_r(errno, buffer.data(), buffer.size()); \
      throw std::runtime_error{ std::format("POSIX: {}", str) }; \
    } \
  } \
  while (0)

#define POSIX_WHINE(exp) \
  do \
  { \
    if ((exp) < 0) \
    { \
      std::cerr << "The expression: \"" #exp "\" failed.\n"; \
    } \
  } \
  while (0)

static constexpr const std::size_t SIGNAL_COUNT{ 64 };

int main() {
  if (!isatty(STDIN_FILENO))
  {
    return 1;
  }
  auto valid = std::uint64_t{ };
  {
    auto all = sigset_t{ };
    POSIX_CHECK(sigemptyset(&all));
    for (auto i = std::size_t{ 1 }; i <= SIGNAL_COUNT; ++i)
    {
      if (sigaddset(&all, i) >= 0)
      {
        valid |= (1ULL << (i - 1));
      }
    }
  }
  auto managed = sigset_t{ };
  POSIX_CHECK(sigfillset(&managed));
  POSIX_CHECK(sigdelset(&managed, SIGKILL));
  POSIX_CHECK(sigdelset(&managed, SIGSTOP));
  POSIX_CHECK(sigdelset(&managed, SIGSEGV));
  POSIX_CHECK(sigdelset(&managed, SIGILL));
  POSIX_CHECK(sigdelset(&managed, SIGBUS));
  POSIX_CHECK(sigdelset(&managed, SIGFPE));
  POSIX_CHECK(sigdelset(&managed, SIGTRAP));
  auto initially_blocked_signals = sigset_t{ };
  POSIX_CHECK(sigprocmask(SIG_BLOCK, &managed, &initially_blocked_signals));
  auto listen_for = sigset_t{ };
  POSIX_CHECK(sigfillset(&listen_for));
  {
    auto is_unique = bool{ };
    auto res = int{ };
    for (auto i = std::size_t{ 1 }; i <= SIGNAL_COUNT; ++i)
    {
      if (valid & (1ULL << (i - 1)))
      {
        is_unique = false;
        POSIX_CHECK(res = sigismember(&initially_blocked_signals, i));
        is_unique = is_unique || res;
        POSIX_CHECK(res = sigismember(&managed, i));
        is_unique = is_unique || !res;
        if (is_unique)
        {
          POSIX_CHECK(sigdelset(&listen_for, i));
        }
      }
    }
  }
  auto signals = signalfd(-1, &listen_for, 0);
  POSIX_CHECK(signals);
  auto polls = std::array<pollfd, 2>{ pollfd{ STDIN_FILENO, POLLIN, 0 }, pollfd{ signals, POLLIN, 0 } };
  auto backup = termios{ };
  POSIX_CHECK(tcgetattr(STDIN_FILENO, &backup));
  auto paused = backup;
  paused.c_lflag &= ~(ICANON | ECHO);
  paused.c_cc[VTIME] = 0;
  paused.c_cc[VMIN] = 1;
  POSIX_CHECK(tcsetattr(STDIN_FILENO, TCSADRAIN, &paused));
  auto res = int{ 0 };
  try
  {
    auto running = true;
    auto ready = int{ };
    while (running)
    {
      POSIX_CHECK((ready = poll(polls.data(), polls.size(), -1)));
      for (auto i = std::size_t{ 0 }; i < polls.size(); ++i)
      {
        if (polls[i].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
          throw std::runtime_error{ std::format("The file descriptor {} encountered an error.", polls[i].fd) };
        }
      }
      if (polls[1].revents & POLLIN)
      {
        POSIX_CHECK(tcsetattr(STDIN_FILENO, TCSADRAIN, &backup));
        POSIX_CHECK(sigprocmask(SIG_SETMASK, &initially_blocked_signals, nullptr));
        POSIX_CHECK(sigprocmask(SIG_BLOCK, &managed, nullptr));
        POSIX_CHECK(tcsetattr(STDIN_FILENO, TCSADRAIN, &paused));
        continue;
      }
      if (polls[0].revents & POLLIN)
      {
        running = false;
      }
    }
  }
  catch (const std::exception& err)
  {
    std::cerr << err.what() << "\n";
    res = 1;
  }
  catch (...)
  {
    std::cerr << "An unknown exception occurred.\n";
    res = 1;
  }
  POSIX_WHINE(tcflush(STDIN_FILENO, TCIFLUSH));
  POSIX_WHINE(tcsetattr(STDIN_FILENO, TCSADRAIN, &backup));
  POSIX_WHINE(sigprocmask(SIG_SETMASK, &initially_blocked_signals, nullptr));
  return res;
}
