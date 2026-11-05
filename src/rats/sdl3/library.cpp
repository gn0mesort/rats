#include "rats/sdl3/library.hpp"

#include <chrono>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include "rats/error.hpp"
#include "rats/logger.hpp"

#define MA_NO_DEVICE_IO
#include <miniaudio.h>

#include "config.hpp"

#define SDL3_CHECK(exp, ...) \
  do \
  { \
    if (!(exp)) \
    { \
      const auto err = std::string{ SDL_GetError() }; \
      __VA_ARGS__ \
      throw rats::error{ rats::error::origin::sdl3, err }; \
    } \
  } \
  while (0)

namespace {

  void process_window_event(rats::sdl3::event& event, SDL_Event& ev) {
    event.window.timestamp = ev.window.timestamp;
    event.window.window_id = ev.window.windowID;
    event.window.data1 = ev.window.data1;
    event.window.data2 = ev.window.data2;
  }

}

namespace rats::sdl3 {

  std::timed_mutex library::sm_lock{ };

  library::library() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only construct an SDL3 library on the main thread." };
    }
    auto lock = std::unique_lock{ library::sm_lock, std::defer_lock };
    if (!lock.try_lock_for(std::chrono::milliseconds{ 1 }))
    {
      throw error{ error::origin::sdl3, "The application couldn't acquire a lock on SDL3." };
    }
    if (SDL_WasInit(0))
    {
      throw error{ error::origin::sdl3, "SDL3 is already initialized elsewhere." };
    }
    SDL3_CHECK(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, CONFIG_MESON_PROJECT_NAME));
    SDL3_CHECK(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_VERSION_STRING, CONFIG_MESON_PROJECT_VERSION_STRING));
    SDL3_CHECK(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Alexander Rothman"));
    SDL3_CHECK(SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game"));
    SDL3_CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD), { SDL_Quit(); });
    SDL3_CHECK(SDL_GL_LoadLibrary(nullptr), { SDL_Quit(); });
    SDL3_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4), { SDL_Quit(); });
    SDL3_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5), { SDL_Quit(); });
    SDL3_CHECK(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE), { SDL_Quit(); });
    SDL3_CHECK(SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, true), { SDL_Quit(); });
  }

  library::~library() noexcept {
    if (!SDL_IsMainThread())
    {
      RATS_CRITICAL(to_string(error::origin::sdl3), "SDL3 is being destroyed from a secondary thread.");
      get_thread_logger().flush();
      std::terminate();
    }
    SDL_Quit();
  }

  std::optional<event> library::poll() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "SDL3 events can only be polled on the main thread." };
    }
    auto ev = SDL_Event{ };
    if (!SDL_PollEvent(&ev))
    {
      return std::nullopt;
    }
    auto res = event{ };
    switch (ev.type)
    {
    case SDL_EVENT_QUIT:
      res.type = event_type::quit;
      break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      res.type = event_type::key;
      res.key.timestamp = ev.key.timestamp;
      res.key.window_id = ev.key.windowID;
      res.key.keyboard_id = ev.key.which;
      res.key.scancode = static_cast<enum scancode>(ev.key.scancode);
      res.key.keycode = static_cast<enum keycode>(ev.key.key);
      res.key.modifiers = ev.key.mod;
      res.key.raw = ev.key.raw;
      res.key.pressed = ev.key.down;
      res.key.repeat = ev.key.repeat;
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      res.type = event_type::mouse_button;
      res.mouse_button.timestamp = ev.button.timestamp;
      res.mouse_button.window_id = ev.button.windowID;
      res.mouse_button.mouse_id = ev.button.which;
      res.mouse_button.x = ev.button.x;
      res.mouse_button.y = ev.button.y;
      res.mouse_button.button = static_cast<enum mouse_button>(ev.button.button);
      res.mouse_button.clicks = ev.button.clicks;
      res.mouse_button.pressed = ev.button.down;
      break;
    case SDL_EVENT_MOUSE_MOTION:
      res.type = event_type::mouse_motion;
      res.mouse_motion.timestamp = ev.motion.timestamp;
      res.mouse_motion.window_id = ev.motion.windowID;
      res.mouse_motion.mouse_id = ev.motion.which;
      res.mouse_motion.state = ev.motion.state;
      res.mouse_motion.x = ev.motion.x;
      res.mouse_motion.y = ev.motion.y;
      res.mouse_motion.relative_x = ev.motion.xrel;
      res.mouse_motion.relative_y = ev.motion.yrel;
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      res.type = event_type::mouse_wheel;
      res.mouse_wheel.timestamp = ev.wheel.timestamp;
      res.mouse_wheel.window_id = ev.wheel.windowID;
      res.mouse_wheel.mouse_id = ev.wheel.which;
      res.mouse_wheel.scroll_x = ev.wheel.x;
      res.mouse_wheel.scroll_y = ev.wheel.y;
      res.mouse_wheel.x = ev.wheel.mouse_x;
      res.mouse_wheel.y = ev.wheel.mouse_y;
      if (ev.wheel.direction == SDL_MOUSEWHEEL_NORMAL)
      {
        res.mouse_wheel.direction = mouse_wheel_direction::normal;
      }
      else
      {
        res.mouse_wheel.direction = mouse_wheel_direction::flipped;
      }
      break;
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
      res.type = event_type::window_focus_gained;
      process_window_event(res, ev);
      break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
      res.type = event_type::window_focus_lost;
      process_window_event(res, ev);
      break;
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
      res.type = event_type::window_mouse_entered;
      process_window_event(res, ev);
      break;
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
      res.type = event_type::window_mouse_left;
      process_window_event(res, ev);
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      res.type = event_type::window_resized;
      process_window_event(res, ev);
      break;
    default:
      res.type = event_type::unhandled;
      break;
    }
    return res;
  }

  extent_2d library::primary_display_resolution() const {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "SDL3 can only query displays on the main thread." };
    }
    const auto id = SDL_GetPrimaryDisplay();
    SDL3_CHECK(id);
    const auto *const mode = SDL_GetDesktopDisplayMode(id);
    SDL3_CHECK(mode);
    return { static_cast<std::uint16_t>(mode->w), static_cast<std::uint16_t>(mode->h) };
  }

}
