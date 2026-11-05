#ifndef RATS_EASING_HPP
#define RATS_EASING_HPP

#include "stopwatch.hpp"
#include "math.hpp"

namespace rats {

  template <typename ValueType>
  class ease_out final {
  public:
    using value_type = ValueType;
    using duration_type = stopwatch::duration_type;
  private:
    duration_type m_acc;
    duration_type m_duration;
    value_type m_start;
    value_type m_end;
    value_type m_current;
  public:
    ease_out(const value_type& start, const value_type& end, const duration_type& duration) :
    m_acc{ 0.0 },
    m_duration{ duration },
    m_start{ start },
    m_end{ end },
    m_current{ start } { }

    ease_out(const value_type& start, const value_type& end, const duration_type::rep duration) :
    ease_out{ start, end, duration_type{ duration } } { }

    ease_out(const ease_out& other) = default;
    ease_out(ease_out&& other) = default;

    ~ease_out() noexcept = default;

    ease_out& operator=(const ease_out& rhs) = default;
    ease_out& operator=(ease_out&& rhs) = default;

    const value_type& operator()() const {
      return m_current;
    }

    void update(const duration_type& dt) {
      m_acc += dt;
      if (m_acc >= m_duration)
      {
        m_current = m_end;
      }
      else
      {
        const auto u = m_acc.count() / m_duration.count();
        m_current = lerp(m_start, m_end, 1 - std::pow(1 - u, 3.0f));
      }
    }

    const value_type& origin() const {
      return m_start;
    }

    const value_type& target() const {
      return m_end;
    }

    const duration_type& duration() const {
      return m_duration;
    }

    bool is_finished() const {
      return m_acc >= m_duration;
    }

    void reset() {
      m_acc = duration_type::zero();
      m_current = m_start;
    }

    void reset(const value_type& start, const value_type& end) {
      reset(start, end, m_duration);
    }

    void reset(const value_type& start, const value_type& end, const duration_type& duration) {
      m_start = start;
      m_end = end;
      m_duration = duration;
      reset();
    }

    void reset(const value_type& start, const value_type& end, const duration_type::rep duration) {
      reset(start, end, duration_type{ duration });
    }
  };

  template <typename ValueType>
  ease_out<ValueType> reverse(const ease_out<ValueType>& ease) {
    auto res = ease_out<ValueType>{ ease(), ease.origin(), ease.duration() };
    return res;
  }

}

#endif
