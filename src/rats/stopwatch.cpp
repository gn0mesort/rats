#include "rats/stopwatch.hpp"

namespace rats {

stopwatch::stopwatch() :
m_t0{ std::chrono::steady_clock::now() } { }


stopwatch::duration_type stopwatch::reset() {
  const auto dt = std::chrono::steady_clock::now() - m_t0;
  m_t0 = std::chrono::steady_clock::now();
  return dt;
}

stopwatch::duration_type stopwatch::elapsed() const {
  return std::chrono::steady_clock::now() - m_t0;
}

}
