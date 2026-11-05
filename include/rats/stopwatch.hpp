#ifndef RATS_STOPWATCH_HPP
#define RATS_STOPWATCH_HPP

#include <chrono>

namespace rats {

  static_assert(std::chrono::steady_clock::period::type::num == 1);
  static_assert(std::chrono::steady_clock::period::type::den >= 1000000);

  class stopwatch final {
  public:
    using duration_type = std::chrono::duration<double>;
    using time_point_type = std::chrono::steady_clock::time_point;
  private:
    time_point_type m_t0;
  public:
    stopwatch();
    stopwatch(const stopwatch& other) = default;
    stopwatch(stopwatch&& other) = default;

    ~stopwatch() noexcept = default;

    stopwatch& operator=(const stopwatch& rhs) = default;
    stopwatch& operator=(stopwatch&& rhs) = default;

    duration_type reset();
    duration_type elapsed() const;
  };

}

#endif
