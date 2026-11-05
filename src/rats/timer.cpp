#include "rats/timer.hpp"

#include <cassert>

namespace rats {

  timer::timer(const double target) :
  timer{ duration_type{ target } } { }

  timer::timer(const duration_type& target) :
  m_target{ target },
  m_acc{ duration_type::zero() } {
    assert(m_target > duration_type::zero());
  }

  void timer::update(const duration_type& dt) {
    m_acc += dt;
  }

  bool timer::consume() {
    if (m_acc >= m_target)
    {
      m_acc -= m_target;
      return true;
    }
    return false;
  }

  timer::duration_type timer::elapsed() const {
    return m_acc;
  }

  void timer::clear() {
    m_acc = timer::duration_type::zero();
  }

}
