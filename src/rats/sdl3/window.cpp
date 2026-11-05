#include "rats/sdl3/window.hpp"

#include <cassert>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include "glad/gl.h"

#include "rats/error.hpp"
#include "rats/logger.hpp"

#include "rats/sdl3/library.hpp"

#define SDL3_CHECK(exp, ...) \
  do \
  { \
    if (!(exp)) \
    { \
      __VA_ARGS__ \
      throw rats::error{ rats::error::origin::sdl3, SDL_GetError() }; \
    } \
  } \
  while (0)

namespace {

  std::string gl_type_to_string(const GLint gltype) {
    switch (gltype)
    {
    case GL_FLOAT:
      return "F";
    case GL_INT:
      return "I";
    case GL_UNSIGNED_INT:
      return "UI";
    case GL_SIGNED_NORMALIZED:
      return "SNORM";
    case GL_UNSIGNED_NORMALIZED:
      return "UNORM";
    default:
      return "";
    }
  }

}

namespace rats::sdl3 {

  struct window::info final {
    SDL_Window* window;
    SDL_GLContext context;
    bool srgb_supported;
  };

  window::window(const std::shared_ptr<library>& parent, const std::string& title, const rect_2d& bounds) :
  m_parent{ parent },
  m_info{ new info{ } } {
    if (!parent)
    {
      throw error{ error::origin::sdl3, "The application can only construct a window from a parent library." };
    }
    if (!bounds.extent.width || !bounds.extent.height)
    {
      throw error{ error::origin::sdl3, "The application can only construct a window with a non-zero area." };
    }
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    m_info->window = SDL_CreateWindow(title.data(), bounds.extent.width, bounds.extent.height,
                                     SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
    SDL3_CHECK(m_info->window != nullptr);
    SDL3_CHECK(SDL_SetWindowPosition(m_info->window, bounds.offset.x, bounds.offset.y),
               { SDL_DestroyWindow(m_info->window); });
    m_info->context = SDL_GL_CreateContext(m_info->window);
    SDL3_CHECK(m_info->context != nullptr, { SDL_DestroyWindow(m_info->window); });
    SDL_SyncWindow(m_info->window);
    assert(m_info->window != nullptr);
    assert(m_info->context != nullptr);
  }

  window::~window() noexcept {
    if (!SDL_IsMainThread())
    {
      RATS_CRITICAL(to_string(error::origin::sdl3), "The application is destroying a window from a secondary thread.");
      get_thread_logger().flush();
      std::terminate();
    }
    hide();
    SDL_GL_DestroyContext(m_info->context);
    SDL_DestroyWindow(m_info->window);
  }

  bool window::change_presentation_mode(const presentation_mode mode) {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    if (SDL_GL_GetCurrentWindow() != m_info->window || SDL_GL_GetCurrentContext() != m_info->context)
    {
      throw error{ error::origin::sdl3,
                   "The application can only change the presentation mode of the current window." };
    }
    auto res = false;
    switch (mode)
    {
    case presentation_mode::immediate:
      res = SDL_GL_SetSwapInterval(0);
      if (res)
      {
        RATS_DEBUG(to_string(rats::error::origin::sdl3), "The OpenGL presentation mode is now Immediate.");
      }
      break;
    case presentation_mode::fifo:
      res = SDL_GL_SetSwapInterval(1);
      if (res)
      {
        RATS_DEBUG(to_string(rats::error::origin::sdl3), "The OpenGL presentation mode is now FIFO.");
      }
      break;
    case presentation_mode::relaxed:
      res = SDL_GL_SetSwapInterval(-1);
      if (res)
      {
        RATS_DEBUG(to_string(rats::error::origin::sdl3), "The OpenGL presentation mode is now FIFO Relaxed.");
      }
      break;
    default:
      res = false;
      break;
    }
    // Apparently, this too can fail. SDL and GL can't guarantee that swap interval 1 works, so fallback to 0, and
    // if that fails give up. If this happens who knows what you get! OpenGL is so cool!!
    if (!res)
    {
      res = SDL_GL_SetSwapInterval(1);
      if (!res)
      {
        res = SDL_GL_SetSwapInterval(0);
        if (!res)
        {
          RATS_WARNING(to_string(rats::error::origin::sdl3),
                       "The application failed to set ANY OpenGL presentation mode! Good luck!");
        }
        else
        {
          RATS_WARNING(to_string(rats::error::origin::sdl3),
                       "The application failed to set the OpenGL presentation mode, so it defaulted to immediate mode.");
        }
      }
      else
      {
        RATS_WARNING(to_string(rats::error::origin::sdl3),
                     "The application failed to set the OpenGL presentation mode, so it defaulted to FIFO mode.");
      }
    }
    return res;
  }

  bool window::is_current() const {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    return SDL_GL_GetCurrentWindow() == m_info->window && SDL_GL_GetCurrentContext() == m_info->context;
  }

  void window::make_current() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL3_CHECK(SDL_GL_MakeCurrent(m_info->window, m_info->context));
    if (!gladLoadGL(SDL_GL_GetProcAddress))
    {
      throw rats::error{ rats::error::origin::glad,
                         "The application failed to load OpenGL command function pointers." };
    }
    SDL3_CHECK(SDL_SyncWindow(m_info->window));
    {
      RATS_INFO(to_string(rats::error::origin::sdl3), "OpenGL Vendor: {}",
                reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
      RATS_INFO(to_string(rats::error::origin::sdl3), "OpenGL Renderer: {}",
                reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
      RATS_INFO(to_string(rats::error::origin::sdl3), "OpenGL Version: {}",
                reinterpret_cast<const char*>(glGetString(GL_VERSION)));
      RATS_INFO(to_string(rats::error::origin::sdl3), "OpenGL Shading Language Version: {}",
                reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
    }
    {
      auto major = GLint64{ };
      glGetInteger64v(GL_MAJOR_VERSION, &major);
      auto minor = GLint64{ };
      glGetInteger64v(GL_MINOR_VERSION, &minor);
      if (major != 4 || minor < 5)
      {
        throw error{ error::origin::sdl3, "The application failed to acquire a complete OpenGL context." };
      }
      RATS_INFO(to_string(rats::error::origin::sdl3), "GL Context Version: {}.{}", major, minor);
    }
    {
      auto status = GLint{ };
      glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &status);
      if (status == GL_NONE)
      {
        throw error{ error::origin::sdl3, "The application failed to acquire a complete OpenGL context." };
      }
      auto r = GLint{ };
      glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &r);
      auto g = GLint{ };
      glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &g);
      auto b = GLint{ };
      glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &b);
      auto a = GLint{ };
      glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE, &a);
      if (r < 8 || g < 8 || b < 8 || a < 8)
      {
        throw error{ error::origin::sdl3, "The application failed to acquire a complete OpenGL context." };
      }
      auto type = GLint{ };
      glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &type);
      auto encoding = GLint{ };
      glGetNamedFramebufferAttachmentParameteriv(0, GL_FRONT_LEFT, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
                                                 &encoding);
      auto ename = std::string{ };
      if (encoding == GL_LINEAR)
      {
        ename = "Linear";
        m_info->srgb_supported = false;
      }
      else
      {
        ename = "sRGB";
        m_info->srgb_supported = true;
      }
      RATS_INFO(to_string(rats::error::origin::sdl3), "GL Color Format: R{}G{}B{}A{}_{} {}", r, g, b, a,
                gl_type_to_string(type), ename);
      glGetNamedFramebufferAttachmentParameteriv(0, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &status);
      if (status != GL_NONE)
      {
        auto d = GLint{ };
        glGetNamedFramebufferAttachmentParameteriv(0, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &d);
        glGetNamedFramebufferAttachmentParameteriv(0, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &type);
        RATS_INFO(to_string(rats::error::origin::sdl3), "GL Depth Format: D{}_{}", d, gl_type_to_string(type));
      }
      glGetNamedFramebufferAttachmentParameteriv(0, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &status);
      if (status != GL_NONE)
      {
        auto s = GLint{ };
        glGetNamedFramebufferAttachmentParameteriv(0, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &s);
        glGetNamedFramebufferAttachmentParameteriv(0, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &type);
        RATS_INFO(to_string(rats::error::origin::sdl3), "GL Stencil Format: S{}_{}", s, gl_type_to_string(type));
      }
    }
    assert(SDL_GL_GetCurrentWindow() == m_info->window);
    assert(SDL_GL_GetCurrentContext() == m_info->context);
  }

  void window::show() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL3_CHECK(SDL_ShowWindow(m_info->window));
    SDL_SyncWindow(m_info->window);
  }

  void window::hide() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL3_CHECK(SDL_HideWindow(m_info->window));
    SDL_SyncWindow(m_info->window);
  }

  void window::present() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL3_CHECK(SDL_GL_SwapWindow(m_info->window));
  }

  rect_2d window::viewport() const {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    auto res = rect_2d{ };
    // Some window systems might not obey requests from the application, so the true viewport is whatever it says
    // it is!
    auto x = int{ };
    auto y = int{ };
    SDL3_CHECK(SDL_GetWindowPosition(m_info->window, &x, &y));
    res.offset.x = x;
    res.offset.y = y;
    auto w = int{ };
    auto h = int{ };
    // This could still not be in pixels, on a high DPI display, so do we care?
    // It would make sense, perhaps to treat those displays as having their alleged sizes, but you could render at
    // a higher total resolution if you used the pixel size instead.
    SDL3_CHECK(SDL_GetWindowSize(m_info->window, &w, &h));
    res.extent.width = w;
    res.extent.height = h;
    return res;
  }

  bool window::is_visible() const {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    return !(SDL_GetWindowFlags(m_info->window) & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED));
  }

  void window::make_cursor_relative() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL3_CHECK(SDL_SetWindowRelativeMouseMode(m_info->window, true));
    RATS_DEBUG(to_string(rats::error::origin::sdl3), "Entered relative mouse mode.");
  }

  void window::make_cursor_absolute() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL3_CHECK(SDL_SetWindowRelativeMouseMode(m_info->window, false));
    RATS_DEBUG(to_string(rats::error::origin::sdl3), "Exited relative mouse mode.");
  }

  void window::make_fullscreen() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL3_CHECK(SDL_SetWindowFullscreen(m_info->window, true));
    SDL_SyncWindow(m_info->window);
    RATS_DEBUG(to_string(rats::error::origin::sdl3), "Entered fullscreen mode.");
  }

  void window::make_windowed() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL3_CHECK(SDL_SetWindowFullscreen(m_info->window, false));
    SDL_SyncWindow(m_info->window);
    RATS_DEBUG(to_string(rats::error::origin::sdl3), "Exited fullscreen mode.");
  }

  void window::toggle_fullscreen() {
    if (!SDL_IsMainThread())
    {
      throw error{ error::origin::sdl3, "The application can only operate on windows from the main thread." };
    }
    SDL_SyncWindow(m_info->window);
    const auto state = (SDL_GetWindowFlags(m_info->window) & SDL_WINDOW_FULLSCREEN) != 0;
    if (state)
    {
      make_windowed();
    }
    else
    {
      make_fullscreen();
    }
  }

  bool window::supports_srgb() const {
    return m_info->srgb_supported;
  }

}
