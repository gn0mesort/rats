#ifndef RATS_RENDERER_HPP
#define RATS_RENDERER_HPP

#include <memory>
#include <string>

#include "geometry.hpp"
#include "graphics.hpp"

namespace rats::sdl3 {

  class window;

}

namespace rats::assets {

  class texture;

}

namespace rats {

  class world;
  class sprite;
  class entity;

  struct camera final {
    vec2 forward;
    vec2 right;
    float plane_length;
  };

  class frame final {
  private:
    struct info;

    friend class renderer;

    info* m_info;

    frame(info& per_frame);
  public:
    frame(frame&& other) = default;

    ~frame() noexcept = default;

    frame& operator=(frame&& rhs) = default;
    rgba& operator[](const vec2& point);
    rgba& operator[](const offset_2d& point);
    const rgba& operator[](const vec2& point) const;
    const rgba& operator[](const offset_2d& point) const;

    const extent_2d& extent() const;

    void clear(const vec3& color);
    void assign_pixel(const offset_2d& pixel, const vec4& color);
    void blend_pixel(const offset_2d& pixel, vec4 color, const bool premultiplied = false);
    void assign_depth(const offset_2d& pixel, const float depth);
    void line(const vec2& from, const vec2& to, const vec4& color, const bool blend = true,
              const bool premultiplied = false);
    void rectangle(const vec2& topleft, const vec2& bottomright, const vec4& color, const bool blend = true,
                   const bool premultiplied = false);
    void blit(const assets::texture& image, const rect_2d& src, const rect_2d& dst, const bool blend = true);
    void text(const vec2& origin, const extent_2d& size, const assets::texture& atlas, const std::string& text, const bool blend = true);
    void filter(const vec4& color);
    void render(const world& w);
    void render(const sprite& spr);
    void render(const world& w, const entity& ent);
    void render_enemy(const world& w, const entity& ent);
  };

  class renderer final {
  private:
    struct info;

    friend std::unique_ptr<sdl3::window> release(std::unique_ptr<renderer>&& render);

    std::unique_ptr<sdl3::window> m_parent;
    std::unique_ptr<info> m_info;
  public:
    renderer(std::unique_ptr<sdl3::window>&& parent, const extent_2d& extent);

    ~renderer() noexcept;
    void resize(const extent_2d& extent);
    void apply_viewport();
    void apply_viewport(const rect_2d& viewport);
    sdl3::window& window();
    const sdl3::window& window() const;
    frame begin_rendering();
    void end_rendering(frame&& finished);
  };

  std::unique_ptr<sdl3::window> release(std::unique_ptr<renderer>&& render);

}

#endif
