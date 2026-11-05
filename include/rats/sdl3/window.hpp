#ifndef RATS_SDL3_WINDOW_HPP
#define RATS_SDL3_WINDOW_HPP

#include <string>
#include <memory>

#include "../geometry.hpp"

namespace rats::sdl3 {

  class library;

  class window final {
  private:
    struct info;

    std::shared_ptr<library> m_parent;
    /* mutable */ std::unique_ptr<info> m_info;
  public:
    enum class presentation_mode {
      immediate,
      fifo,
      relaxed
    };

    window(const std::shared_ptr<library>& parent, const std::string& title, const rect_2d& bounds);
    window(const window& other) = delete;
    window(window&&) = default;

    ~window() noexcept;

    window& operator=(const window& rhs) = delete;
    window& operator=(window&& rhs) = default;

    bool change_presentation_mode(const presentation_mode mode);
    bool is_current() const;
    void make_current();
    void show();
    void hide();
    void present();
    rect_2d viewport() const;
    bool is_visible() const;
    void make_cursor_relative();
    void make_cursor_absolute();
    void make_fullscreen();
    void make_windowed();
    void toggle_fullscreen();
    bool supports_srgb() const;
  };

}

#endif
