#ifndef RATS_TIMER_HPP
#define RATS_TIMER_HPP

#include <chrono>

namespace rats {

  class timer final {
  public:
    using duration_type = std::chrono::duration<double>;
  private:
    duration_type m_target;
    duration_type m_acc;
  public:
    explicit timer(const double target);
    explicit timer(const duration_type& target);
    timer(const timer& other) = default;
    timer(timer&& other) = default;

    ~timer() noexcept = default;

    timer& operator=(const timer& rhs) = default;
    timer& operator=(timer&& rhs) = default;

    void update(const duration_type& dt);
    bool consume();
    duration_type elapsed() const;
    void clear();
  };

}

#endif
