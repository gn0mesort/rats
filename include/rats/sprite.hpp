#ifndef RATS_SPRITE_HPP
#define RATS_SPRITE_HPP

#include <cstddef>

#include <unordered_map>
#include <string>

#include "geometry.hpp"
#include "math.hpp"
#include "assets.hpp"
#include "stopwatch.hpp"

namespace rats {

  class sprite final {
  public:
    struct animation final {
      stopwatch::duration_type duration;
      rect_2d region;
      extent_2d frame_extent;
      std::size_t frames;
    };
  private:
    assets::texture m_sheet;
    std::unordered_map<std::string, animation> m_animations;
    std::string m_current;
    rect_2d m_frame;
    stopwatch::duration_type m_acc;
    stopwatch::duration_type m_seconds_per_frame;
    vec2 m_position;
    bool m_looping;
    bool m_playing;
  public:
    sprite(const assets::texture& sheet, const vec2& position);
    sprite(const sprite& other) = default;

    ~sprite() noexcept = default;

    sprite& operator=(const sprite& rhs) = default;

    void intern_animation(const std::string& name, const animation& anim);
    void play(const std::string& animation, const bool loop = false);
    void stop();
    void update(const stopwatch::duration_type dt);
    const assets::texture& sheet() const;
    const rect_2d& frame() const;
    const vec2& position() const;
  };

}

#endif
