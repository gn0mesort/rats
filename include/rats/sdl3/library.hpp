#ifndef RATS_SDL3_LIBRARY_HPP
#define RATS_SDL3_LIBRARY_HPP

#include <cinttypes>

#include <optional>
#include <mutex>

#include "inputs.hpp"
#include "../geometry.hpp"

namespace rats::sdl3 {

  enum class event_type {
    unhandled,
    quit,
    key,
    mouse_motion,
    mouse_button,
    mouse_wheel,
    window_focus_gained,
    window_focus_lost,
    window_mouse_entered,
    window_mouse_left,
    window_resized
  };

  struct key_event final {
    std::uint64_t timestamp;
    std::uint32_t window_id;
    std::uint32_t keyboard_id;
    enum scancode scancode;
    enum keycode keycode;
    std::uint64_t modifiers;
    std::uint16_t raw;
    bool pressed;
    bool repeat;
  };

  struct mouse_motion_event final {
    std::uint64_t timestamp;
    std::uint32_t window_id;
    std::uint32_t mouse_id;
    std::uint32_t state;
    float x;
    float y;
    float relative_x;
    float relative_y;
    unsigned char pad0[4];
  };

  struct mouse_button_event final {
    std::uint64_t timestamp;
    std::uint32_t window_id;
    std::uint32_t mouse_id;
    float x;
    float y;
    enum mouse_button button;
    std::uint8_t clicks;
    bool pressed;
    unsigned char pad0[5];
  };

  enum class mouse_wheel_direction : std::int32_t {
    flipped = -1,
    normal = 1
  };

  struct mouse_wheel_event final {
    std::uint64_t timestamp;
    std::uint32_t window_id;
    std::uint32_t mouse_id;
    float scroll_x;
    float scroll_y;
    float x;
    float y;
    mouse_wheel_direction direction;
    unsigned char pad0[4];
  };

  struct window_event final {
    std::uint64_t timestamp;
    std::uint32_t window_id;
    std::int32_t data1;
    std::int32_t data2;
    unsigned char pad0[4];
  };

  struct event final {
    event_type type;
    union {
      key_event key;
      mouse_motion_event mouse_motion;
      mouse_button_event mouse_button;
      mouse_wheel_event mouse_wheel;
      window_event window;
    };
  };

  class library final {
  private:
    static std::timed_mutex sm_lock;
  public:
    library();
    library(const library& other) = delete;
    library(library&& other) = delete;

    ~library() noexcept;

    library& operator=(const library& rhs) = delete;
    library& operator=(library&& rhs) = delete;

    std::optional<event> poll();
    extent_2d primary_display_resolution() const;
  };

}

#endif
