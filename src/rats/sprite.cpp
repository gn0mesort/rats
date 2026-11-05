#include "rats/sprite.hpp"

#include "rats/error.hpp"

namespace rats {

  sprite::sprite(const assets::texture& sheet, const vec2& position) :
  m_sheet{ sheet },
  m_current{ "" },
  m_frame{ { 0, 0 }, sheet.extent() },
  m_acc{ stopwatch::duration_type::zero() },
  m_seconds_per_frame{ stopwatch::duration_type::zero() },
  m_position{ position },
  m_looping{ false },
  m_playing{ false } { }

  void sprite::intern_animation(const std::string& name, const animation& anim) {
    if (name.empty())
    {
      throw error{ error::origin::application, "All animations must have a name." };
    }
    m_animations[name] = anim;
  }

  void sprite::play(const std::string& animation, const bool loop) {
    if (m_playing)
    {
      stop();
    }
    if (animation.empty() || !m_animations.contains(animation))
    {
      throw error{ error::origin::application, "The application couldn't find the desired animation" };
    }
    m_current = animation;
    const auto& anim = m_animations[m_current];
    m_frame.offset = anim.region.offset;
    m_frame.extent = anim.frame_extent;
    m_seconds_per_frame = anim.duration / anim.frames;
    m_playing = true;
    m_looping = loop;
  }

  void sprite::stop() {
    if (m_playing)
    {
      m_playing = false;
      m_looping = false;
      m_acc = stopwatch::duration_type::zero();
      const auto& anim = m_animations[m_current];
      m_frame.offset = anim.region.offset;
      m_frame.extent = anim.frame_extent;
    }
  }

  void sprite::update(const stopwatch::duration_type dt) {
    if (m_playing)
    {
      m_acc += dt;
      const auto* animation = static_cast<const struct animation*>(nullptr);
      while (m_acc >= m_seconds_per_frame)
      {
        if (m_playing)
        {
          animation = &m_animations[m_current];
          m_frame.offset.x += m_frame.extent.width;
          if (m_frame.offset.x >= animation->region.extent.width)
          {
            m_frame.offset.x = animation->region.offset.x;
            if (!m_looping)
            {
              m_playing = false;
            }
          }
        }
        m_acc -= m_seconds_per_frame;
      }
    }
  }

  const assets::texture& sprite::sheet() const {
    return m_sheet;
  }

  const rect_2d& sprite::frame() const {
    return m_frame;
  }

  const vec2& sprite::position() const {
    return m_position;
  }

}
