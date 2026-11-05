#include "rats/logger.hpp"

#include "config.hpp"

#include <format>

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

#ifdef CONFIG_MESON_SYSTEM_LINUX
#  include <unistd.h>
#elifdef CONFIG_MESON_SYSTEM_WINDOWS
#  include <Windows.h>
#endif

#include "rats/error.hpp"

namespace {

  bool is_terminal(const boost::iostreams::file_descriptor_sink& fdsink) {
#ifdef CONFIG_MESON_SYSTEM_LINUX
    return isatty(fdsink.handle());
#elifdef CONFIG_MESON_SYSTEM_WINDOWS
    auto tty = false;
    if (fdsink.handle() != INVALID_HANDLE_VALUE)
    {
      tty = GetFileType(fdsink.handle()) == FILE_TYPE_CHAR;
      auto mode = DWORD{ };
      if (GetConsoleMode(fdsink.handle(), &mode))
      {
        tty = tty && (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING);
      }
    }
    return tty;
#else
    return false;
#endif
  }

  std::string color_from_severity(const rats::logger::message_severity severity) {
    using enum rats::logger::message_severity;
    switch (severity)
    {
    case critical:
    case error:
      return "\x1b[31m";
    case warning:
      return "\x1b[33m";
    case debug:
      return "\x1b[36m";
    default:
      return "\x1b[0m";
    }
  }

}

namespace rats {

  logger::logger(const std::filesystem::path& target, const message_severity severity) :
  m_write_index{ LOGGER_BUFFER_LENGTH },
  m_insert_index{ 0 },
  m_max_severity{ severity },
  m_buffer{ new std::array<message, LOGGER_BUFFER_LENGTH>{ } } {
    m_files.emplace_back(target, std::ios::binary);
    m_files.back().exceptions(std::ios::failbit);
    m_sinks.emplace_back(&m_files.back());
    if (!m_files.back().good())
    {
      throw rats::error{ rats::error::origin::application, "The application failed to open the target file." };
    }
  }

  logger::logger(const std::filesystem::path& target) : logger{ target, message_severity::info } { }

  logger::logger(std::ostream& target, const message_severity severity) :
  m_write_index{ LOGGER_BUFFER_LENGTH },
  m_insert_index{ 0 },
  m_max_severity{ severity },
  m_buffer{ new std::array<message, LOGGER_BUFFER_LENGTH>{ } } {
    m_sinks.emplace_back(&target);
  }

  logger::logger(std::ostream& target) : logger{ target, message_severity::info } { }

  logger::~logger() noexcept {
    flush();
  }

#define BUFFER_NEXT(index) (((index) + 1) & (LOGGER_BUFFER_LENGTH - 1))

  void logger::log(const message_severity severity, const std::string& origin, const std::string& message) {
    if (m_insert_index == m_write_index)
    {
      flush();
    }
    if (m_write_index >= LOGGER_BUFFER_LENGTH)
    {
      m_write_index = m_insert_index;
    }
    auto& record = (*m_buffer)[m_insert_index];
    record.severity = severity;
    record.time = std::chrono::system_clock::now();
    std::snprintf(record.origin.data(), LOGGER_MAX_LOGGER_ORIGIN_LENGTH, "%s", origin.data());
    std::snprintf(record.content.data(), LOGGER_MAX_MESSAGE_LENGTH, "%s", message.data());
    m_insert_index = BUFFER_NEXT(m_insert_index);
  }

  void logger::flush() {
    if (m_write_index >= LOGGER_BUFFER_LENGTH)
    {
      return;
    }
    do
    {
      const auto& record = (*m_buffer)[m_write_index];
      if (record.severity <= m_max_severity)
      {
        using boost::iostreams::file_descriptor_sink;
        auto* fdstream = static_cast<boost::iostreams::stream<file_descriptor_sink>*>(nullptr);
        const auto* fdsink = static_cast<const file_descriptor_sink*>(nullptr);
        auto use_color = false;
        for (auto *const sink : m_sinks)
        {
          fdstream = dynamic_cast<boost::iostreams::stream<file_descriptor_sink>*>(sink);
          fdsink = fdstream ? &(**fdstream) : nullptr;
          use_color = fdsink ? is_terminal(*fdsink) : false;
          (*sink) << "["
                  << std::format("{:%F %R:%S%z}", std::chrono::time_point_cast<std::chrono::milliseconds>(record.time))
                  << "] [" << CONFIG_MESON_PROJECT_NAME << "] [" << record.origin.data() << "] [";
          if (use_color)
          {
            (*sink) << color_from_severity(record.severity);
          }
          (*sink) << to_string(record.severity);
          if (use_color)
          {
            (*sink) << "\x1b[0m";
          }
          (*sink) << "]: " << record.content.data() << "\n";
        }
      }
      m_write_index = BUFFER_NEXT(m_write_index);
    }
    while (m_write_index != m_insert_index);
    for (auto *const sink : m_sinks)
    {
      sink->flush();
    }
    m_write_index = LOGGER_BUFFER_LENGTH;
  }

  void logger::critical(const std::string& origin, const std::string& message) {
    log(message_severity::critical, origin, message);
  }

  void logger::error(const std::string& origin, const std::string& message) {
    log(message_severity::error, origin, message);
  }

  void logger::warning(const std::string& origin, const std::string& message) {
    log(message_severity::warning, origin, message);
  }

  void logger::info(const std::string& origin, const std::string& message) {
    log(message_severity::info, origin, message);
  }

  void logger::debug(const std::string& origin, const std::string& message) {
    log(message_severity::debug, origin, message);
  }

  void logger::add_sink(const std::filesystem::path& target) {
    m_files.emplace_back(target, std::ios::binary);
    m_files.back().exceptions(std::ios::failbit);
    m_sinks.emplace_back(&m_files.back());
    if (!m_files.back().good())
    {
      throw rats::error{ rats::error::origin::application, "The application failed to open the target file." };
    }
  }

  void logger::add_sink(std::ostream& target) {
    m_sinks.emplace_back(&target);
  }

  std::string to_string(const logger::message_severity severity) {
    using enum logger::message_severity;
    switch (severity)
    {
    case critical:
      return "Critical";
    case error:
      return "Error";
    case warning:
      return "Warning";
    case info:
      return "Info";
    case debug:
      return "Debug";
    default:
      return "Unknown";
    }
  }

  logger& get_thread_logger() {
    using boost::iostreams::file_descriptor_sink;
    static thread_local std::unique_ptr<boost::iostreams::stream<file_descriptor_sink>> s_tl_log;
    static thread_local std::unique_ptr<logger> s_tl_instance;
    if (!s_tl_log && !s_tl_instance)
    {
#ifdef CONFIG_MESON_SYSTEM_LINUX
      auto *const ptr = new boost::iostreams::stream<file_descriptor_sink>{ STDERR_FILENO,
                                                                            boost::iostreams::never_close_handle };
      s_tl_log.reset(ptr);
#elifdef CONFIG_MESON_SYSTEM_WINDOWS
      auto *const ptr = new boost::iostreams::stream<file_descriptor_sink>{ GetStdHandle(STD_ERROR_HANDLE),
                                                                           boost::iostreams::never_close_handle };
      s_tl_log.reset(ptr);
#else
      throw rats::error{ rats::error::application, "The host platform doesn't support a global logger." };
#endif
#ifdef NDEBUG
      s_tl_instance.reset(new logger{ *s_tl_log });
#else
      s_tl_instance.reset(new logger{ *s_tl_log, logger::message_severity::debug });
#endif
    }
    return *s_tl_instance;
  }

}
