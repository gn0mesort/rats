#ifndef RATS_ERROR_HPP
#define RATS_ERROR_HPP

#include <exception>
#include <string>

#define ERROR_ORIGINS \
  ERROR_ORIGIN(application) \
  ERROR_ORIGIN(sdl3) \
  ERROR_ORIGIN(glad)

#define ERROR_ORIGIN(x) x,

namespace rats {

  class error final : public std::exception {
  public:
    enum class origin {
      ERROR_ORIGINS
    };
  private:
    origin m_origin;
    std::string m_message;
  public:
    error(const enum origin origin, const std::string& message);
    explicit error(const std::string& message);
    error(const error& other) = default;
    error(error&& other) = default;

    ~error() noexcept = default;

    error& operator=(const error& rhs) = default;
    error& operator=(error&& rhs) = default;

    const char* what() const noexcept override;
    const std::string& message() const noexcept;
    enum origin origin() const noexcept;
  };

  std::string to_string(const enum error::origin origin);

}

#undef ERROR_ORIGIN

#endif
