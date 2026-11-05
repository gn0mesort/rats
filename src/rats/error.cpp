#include "rats/error.hpp"

namespace rats {

error::error(const enum origin origin, const std::string& message) :
m_origin{ origin },
m_message{ message } { }

error::error(const std::string& message) :
error{ origin::application, message } { }

const char* error::what() const noexcept {
  return m_message.data();
}

const std::string& error::message() const noexcept {
  return m_message;
}

enum error::origin error::origin() const noexcept {
  return m_origin;
}

#define ERROR_ORIGIN(x) case error::origin::x: return #x;

std::string to_string(const enum error::origin origin) {
  switch (origin)
  {
  ERROR_ORIGINS
  }
  return "unknown";
}

#undef ERROR_ORIGINS

}
