#ifndef RATS_LOGGER_HPP
#define RATS_LOGGER_HPP

#include <cstddef>

#include <array>
#include <vector>
#include <fstream>
#include <iosfwd>
#include <memory>
#include <chrono>
#include <filesystem>
#include <string>

#define RATS_CRITICAL(origin, message, ...) \
  rats::get_thread_logger().critical((origin), (message) __VA_OPT__(,) __VA_ARGS__)

#define RATS_ERROR(origin, message, ...) \
  rats::get_thread_logger().error((origin), (message) __VA_OPT__(,) __VA_ARGS__)

#define RATS_WARNING(origin, message, ...) \
  rats::get_thread_logger().warning((origin), (message) __VA_OPT__(,) __VA_ARGS__)

#define RATS_INFO(origin, message, ...) \
  rats::get_thread_logger().info((origin), (message) __VA_OPT__(,) __VA_ARGS__)

#define RATS_DEBUG(origin, message, ...) \
  rats::get_thread_logger().debug((origin), (message) __VA_OPT__(,) __VA_ARGS__)

namespace rats {

constexpr std::size_t LOGGER_MAX_LOGGER_ORIGIN_LENGTH{ 32 };
constexpr std::size_t LOGGER_MAX_MESSAGE_LENGTH{ 2048 };
constexpr std::size_t LOGGER_BUFFER_LENGTH{ 32 };

class logger final {
public:
    enum class message_severity : std::size_t {
      critical = 0,
      error = 1,
      warning = 2,
      info = 3,
      debug = 4,
      verbose = debug,
      trace = debug,
      fatal = critical
    };
private:
  struct message {
    message_severity severity;
    std::chrono::system_clock::time_point time;
    std::array<char, LOGGER_MAX_LOGGER_ORIGIN_LENGTH> origin;
    std::array<char, LOGGER_MAX_MESSAGE_LENGTH> content;
  };
  std::size_t m_write_index;
  std::size_t m_insert_index;
  message_severity m_max_severity;
  std::vector<std::ostream*> m_sinks;
  std::vector<std::ofstream> m_files;
  std::unique_ptr<std::array<message, LOGGER_BUFFER_LENGTH>> m_buffer;
public:
  logger(const std::filesystem::path& target, const message_severity severity);
  logger(const std::filesystem::path& target);
  logger(std::ostream& target, const message_severity severity);
  logger(std::ostream& target);

  ~logger() noexcept;

  void log(const message_severity severity, const std::string& origin, const std::string& message);
  void flush();
  void critical(const std::string& origin, const std::string& message);
  void error(const std::string& origin, const std::string& message);
  void warning(const std::string& origin, const std::string& message);
  void info(const std::string& origin, const std::string& message);
  void debug(const std::string& origin, const std::string& message);
  void add_sink(const std::filesystem::path& target);
  void add_sink(std::ostream& target);

  template <typename... Args>
  void critical(const std::string& origin, const std::format_string<Args...>& message, Args&&... args) {
    critical(origin, std::vformat(message.get(), std::make_format_args(args...)));
  }

  template <typename... Args>
  void error(const std::string& origin, const std::format_string<Args...>& message, Args&&... args) {
    error(origin, std::vformat(message.get(), std::make_format_args(args...)));
  }

  template <typename... Args>
  void warning(const std::string& origin, const std::format_string<Args...>& message, Args&&... args) {
    warning(origin, std::vformat(message.get(), std::make_format_args(args...)));
  }

  template <typename... Args>
  void info(const std::string& origin, const std::format_string<Args...>& message, Args&&... args) {
    info(origin, std::vformat(message.get(), std::make_format_args(args...)));
  }

  template <typename... Args>
  void debug(const std::string& origin, const std::format_string<Args...>& message, Args&&... args) {
    debug(origin, std::vformat(message.get(), std::make_format_args(args...)));
  }
};

std::string to_string(const logger::message_severity severity);

logger& get_thread_logger();

}

#endif
