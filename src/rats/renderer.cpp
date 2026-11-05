#include "rats/renderer.hpp"

#include <cassert>
#include <cstddef>
#include <cinttypes>
#include <cstring>
#include <cmath>

#include <array>
#include <vector>
#include <limits>
#include <algorithm>

#include "glad/gl.h"

#include "rats/error.hpp"
#include "rats/sdl3/window.hpp"
#include "rats/math.hpp"
#include "rats/world.hpp"
#include "rats/assets.hpp"
#include "rats/sprite.hpp"

namespace rats {

  constexpr const std::size_t FRAMES_IN_FLIGHT{ 2 };

  struct frame::info {
    std::vector<rgba>* pixels;
    std::vector<float>* depth;
    std::size_t frame_id;
    extent_2d* extent;
  };

  frame::frame(info& per_frame) :
  m_info{ &per_frame } {
    assert(m_info != nullptr);
    assert(m_info->pixels != nullptr);
    assert(m_info->extent->width > 0);
    assert(m_info->extent->height > 0);
  }

  rgba& frame::operator[](const vec2& point) {
    const auto index = static_cast<std::int64_t>(point.y) * m_info->extent->width + static_cast<std::int64_t>(point.x);
    return (*m_info->pixels)[index];
  }

  rgba& frame::operator[](const offset_2d& point) {
    const auto index = static_cast<std::int64_t>(point.y) * m_info->extent->width + static_cast<std::int64_t>(point.x);
    return (*m_info->pixels)[index];
  }

  const rgba& frame::operator[](const vec2& point) const {
    const auto index = static_cast<std::int64_t>(point.y) * m_info->extent->width + static_cast<std::int64_t>(point.x);
    return (*m_info->pixels)[index];
  }

  const rgba& frame::operator[](const offset_2d& point) const {
    const auto index = static_cast<std::int64_t>(point.y) * m_info->extent->width + static_cast<std::int64_t>(point.x);
    return (*m_info->pixels)[index];
  }

  const extent_2d& frame::extent() const {
    return *m_info->extent;
  }

  void frame::clear(const vec3& color) {
    const auto bincolor = denormalized({ color.x, color.y, color.z, 1.0f });
    std::fill(m_info->pixels->begin(),
              m_info->pixels->end(),
              bincolor);
  }

  void frame::assign_pixel(const offset_2d& pixel, const vec4& color) {
    if (pixel.x < 0 || pixel.x >= m_info->extent->width || pixel.y < 0 || pixel.y >= m_info->extent->height)
    {
      return;
    }
    if (is_almost_zero(color.w, 1.0f / 512.0f))
    {
      return;
    }
    const auto index = static_cast<std::size_t>(pixel.y) * m_info->extent->width + static_cast<std::size_t>(pixel.x);
    (*m_info->pixels)[index] = denormalized(color);
    (*m_info->pixels)[index].a = std::numeric_limits<unsigned char>::max();
  }

  void frame::assign_depth(const offset_2d& pixel, const float depth) {
    if (pixel.x < 0 || pixel.x >= m_info->extent->width || pixel.y < 0 || pixel.y >= m_info->extent->height)
    {
      return;
    }
    const auto index = static_cast<std::size_t>(pixel.y) * m_info->extent->width + static_cast<std::size_t>(pixel.x);
    (*m_info->depth)[index] = std::clamp(depth, -1.0f, 1.0f);
  }

  void frame::blend_pixel(const offset_2d& pixel, vec4 color, const bool premultiplied) {
    if (pixel.x < 0 || pixel.x >= m_info->extent->width || pixel.y < 0 || pixel.y >= m_info->extent->height)
    {
      return;
    }
    const auto index = static_cast<std::size_t>(pixel.y) * m_info->extent->width + static_cast<std::size_t>(pixel.x);
    if (!premultiplied)
    {
      color = premultiply_alpha(color);
    }
    const auto current = normalized((*m_info->pixels)[index]);
    color.x = color.x + current.x * (1.0f - color.w);
    color.y = color.y + current.y * (1.0f - color.w);
    color.z = color.z + current.z * (1.0f - color.w);
    color.w = 1.0f;
    (*m_info->pixels)[index] = denormalized(color);
  }

  void frame::line(const vec2& from, const vec2& to, const vec4& color, const bool blend, const bool premultiplied) {
    const auto dx = std::abs(static_cast<std::int64_t>(to.x) - static_cast<std::int64_t>(from.x));
    const auto dy = -std::abs(static_cast<std::int64_t>(to.y) - static_cast<std::int64_t>(from.y));
    const auto sx = from.x < to.x ? std::int64_t{ 1 } : std::int64_t{ -1 };
    const auto sy = from.y < to.y ? std::int64_t{ 1 } : std::int64_t{ -1 };
    auto x = static_cast<std::int64_t>(from.x);
    auto y = static_cast<std::int64_t>(from.y);
    auto e0 = dx + dy;
    auto e1 = std::int64_t{ };
    while (true)
    {
      if (x >= 0 && x < m_info->extent->width && y >= 0 && y < m_info->extent->height)
      {
        if (blend)
        {
          blend_pixel({ static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) }, color, premultiplied);
        }
        else
        {
          assign_pixel({ static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) }, color);
        }
      }
      if (x == static_cast<std::int64_t>(to.x) && y == static_cast<std::int64_t>(to.y))
      {
        break;
      }
      e1 = 2 * e0;
      if (e1 >= dy)
      {
        e0 += dy;
        x += sx;
      }
      if (e1 <= dx)
      {
        e0 += dx;
        y += sy;
      }
    }
  }

  void frame::rectangle(const vec2& topleft, const vec2& bottomright, const vec4& color, const bool blend,
                        const bool premultiplied) {
    const auto left = std::clamp(static_cast<std::int64_t>(topleft.x), std::int64_t{ 0 },
                                 static_cast<std::int64_t>(m_info->extent->width - 1));
    const auto top = std::clamp(static_cast<std::int64_t>(topleft.y), std::int64_t{ 0 },
                                static_cast<std::int64_t>(m_info->extent->height - 1));
    const auto right = std::clamp(static_cast<std::int64_t>(bottomright.x), std::int64_t{ 0 },
                                  static_cast<std::int64_t>(m_info->extent->width - 1));
    const auto bottom = std::clamp(static_cast<std::int64_t>(bottomright.y), std::int64_t{ 0 },
                                   static_cast<std::int64_t>(m_info->extent->height - 1));
    const auto width = static_cast<std::size_t>(right - left + 1);
    const auto height = static_cast<std::size_t>(bottom - top + 1);
    if (!blend)
    {
      auto bincolor = denormalized(color);
      bincolor.a = 255;
      for (auto i = std::size_t{ 0 }; i < width; ++i)
      {
        (*m_info->pixels)[top * m_info->extent->width + left + i] = bincolor;
      }
      const auto *const first = &(*m_info->pixels)[top * m_info->extent->width + left];
      auto* row = static_cast<rgba*>(nullptr);
      for (auto i = std::size_t{ 1 }; i < height; ++i)
      {
        row = &(*m_info->pixels)[(top + i) * m_info->extent->width + left];
        std::memcpy(row, first, width * sizeof(rgba));
      }
    }
    else
    {
      for (auto i = std::size_t{ 0 }; i < height; ++i)
      {
        for (auto j = std::size_t{ 0 }; j < width; ++j)
        {
          blend_pixel({ static_cast<std::int16_t>(left + j), static_cast<std::int16_t>(top + i) }, color,
                      premultiplied);
        }
      }
    }
  }

  void frame::blit(const assets::texture& image, const rect_2d& src, const rect_2d& dst, const bool blend) {
    auto scoords = offset_2d{ };
    auto dcoords = offset_2d{ };
    for (auto i = std::int32_t{ 0 }; i < dst.extent.height; ++i)
    {
      for (auto j = std::int32_t{ 0 }; j < dst.extent.width; ++j)
      {
        scoords.x = std::clamp(src.offset.x + (j * src.extent.width) / dst.extent.width, 0,
                               static_cast<std::int32_t>(image.extent().width - 1));
        scoords.y = std::clamp(src.offset.y + (i * src.extent.height) / dst.extent.height, 0,
                               static_cast<std::int32_t>(image.extent().height - 1));
        dcoords.x = dst.offset.x + j;
        dcoords.y = dst.offset.y + i;
        if (dcoords.x < 0 || dcoords.x >= m_info->extent->width || dcoords.y < 0 ||
            dcoords.y >= m_info->extent->height)
        {
          continue;
        }
        if (blend)
        {
          blend_pixel(dcoords, normalized(image[scoords]), image.has_premultiplied_alpha());
        }
        else
        {
          assign_pixel(dcoords, normalized(image[scoords]));
        }
      }
    }
  }

  void frame::filter(const vec4& color) {
    auto pre = premultiply_alpha(color);
    for (auto i = std::int16_t{ 0 }; i < m_info->extent->height; ++i)
    {
      for (auto j = std::int16_t{ 0 }; j < m_info->extent->width; ++j)
      {
        blend_pixel({ j, i }, pre, true);
      }
    }
  }

  void frame::text(const vec2& origin, const extent_2d& size, const assets::texture& atlas, const std::string& text,
                   const bool blend) {
    auto src = rect_2d{ { 0, 0 }, { 8, 16 } };
    auto dst = rect_2d{ { static_cast<std::int16_t>(origin.x), static_cast<std::int16_t>(origin.y) }, size };
    for (const auto c : text)
    {
      // Printable ASCII, anything else gets clamped to space or DEL (also a space)
      src.offset.x = std::clamp(static_cast<std::int32_t>(c - ' '), 0, 95) * src.extent.width;
      blit(atlas, src, dst, blend);
      dst.offset.x += dst.extent.width;
    }
  }

  void frame::render(const world& w) {
    if (w.is_showing_automap())
    {
      const auto& forward = w.player().forward();
      const auto& right = w.player().right();
      {
        auto cx = 0.0f;
        auto direction = vec2{ };
        for (auto i = std::int32_t{ 0 }; i < m_info->extent->width; ++i)
        {
          cx = 2.0f * i / (m_info->extent->width - 1) - 1.0f;
          direction.x = forward.x + right.x * w.camera_plane() * cx;
          direction.y = forward.y + right.y * w.camera_plane() * cx;
          w.wallcast({ w.player().position(), direction, std::sqrt(8.0f) });
        }
      }
      const auto extent = w.extent();
      const auto* cell = static_cast<const world::cell*>(nullptr);
      auto x = float{ };
      auto y = float{ };
      auto cx = float{ };
      auto cy = float{ };
      const auto black = normalized(world::palette().color(0));
      const auto player = offset_2d{ static_cast<std::int16_t>(w.player().position().x),
                                     static_cast<std::int16_t>(w.player().position().y) };
      const auto gleft = std::max(player.x - 8, 0);
      const auto gright = std::min(gleft + 15, extent.width - 1);
      const auto gtop = std::max(player.y - 8, 0);
      const auto gbottom = std::min(gtop + 15, extent.height - 1);
      const auto mleft = (m_info->extent->width - 128) >> 1;
      const auto mtop = (m_info->extent->height - 128) >> 1;
      clear({ black.x, black.y, black.z });
      for (auto i = gtop; i <= gbottom; ++i)
      {
        for (auto j = gleft; j <= gright; ++j)
        {

          cell = &w.at({ static_cast<std::int16_t>(j), static_cast<std::int16_t>(i) });
          x = mleft + (j - gleft) * 8.0f + 0.5f;
          y = mtop + (i - gtop) * 8.0f + 0.5f;
          if (cell->flags & world::CELL_FLAG_SEEN_BIT)
          {
            if (cell->wall)
            {
              rectangle({ x, y }, { x + 7.0f, y + 7.0f }, normalized(world::palette().color(2)), false);
            }
            else
            {
              rectangle({ x, y }, { x + 7.0f, y + 7.0f }, normalized(world::palette().color(1)), false);
            }
          }
          if (j == player.x && i == player.y)
          {
            cx = x + 4.0f;
            cy = y + 4.0f;
            line({ cx - 3.0f * forward.x, cy - 3.0f * forward.y }, { cx, cy }, normalized(world::palette().color(3)),
                 false);
            line({ cx, cy }, { cx + 3.0f * forward.x, cy + 3.0f * forward.y }, normalized(world::palette().color(3)),
                 false);
            line({ cx + 3.0f * forward.x, cy + 3.0f * forward.y }, { cx + 2.0f * right.x, cy + 2.0f * right.y },
                 normalized(world::palette().color(3)), false);
            line({ cx + 3.0f * forward.x, cy + 3.0f * forward.y }, { cx - 2.0f * right.x, cy - 2.0f * right.y },
                 normalized(world::palette().color(3)), false);
          }
        }
      }
      return;
    }
    auto cx = 0.0f;
    auto hit = std::optional<wall_hit>{ };
    auto wall_height = float{ };
    auto wall_top = float{ };
    auto wall_bottom = float{ };
    auto u = 0.0f;
    auto v = 0.0f;
    auto dv = 0.0f;
    auto y = std::int32_t{ 0 };
    auto direction = vec2{ };
    auto distance_to_horizon = 0.0f;
    auto wx = 0.0f;
    auto wy = 0.0f;
    auto color = vec4{ };
    auto level = 0.5f - 0.015f * (1.0f - std::cos(w.player_light_level()));
    auto light = 0.0f;
    const auto& position = w.player().position();
    const auto& forward = w.player().forward();
    const auto& right = w.player().right();
    const auto max_d = std::sqrt(18.0f);
    for (auto i = std::int32_t{ 0 }; i < m_info->extent->width; ++i)
    {
      cx = 2.0f * i / (m_info->extent->width - 1) - 1.0f;
      direction.x = forward.x + right.x * w.camera_plane() * cx;
      direction.y = forward.y + right.y * w.camera_plane() * cx;
      hit = w.wallcast({ position, direction, max_d });
      if (hit)
      {
        wall_height = m_info->extent->height / hit->distance;
        wall_top = (m_info->extent->height - wall_height) / 2.0f;
        wall_bottom = std::min(wall_top + wall_height, static_cast<float>(m_info->extent->height));
      }
      else
      {
        wall_height = 0.0f;
        wall_top = m_info->extent->height / 2.0f;
        wall_bottom = wall_top;
      }
      for (y = 0; y < static_cast<std::int32_t>(wall_top); ++y)
      {
        {
          distance_to_horizon = 0.5f * m_info->extent->height / std::fabs((y + 0.5f) - m_info->extent->height * 0.5f);
          wx = position.x + direction.x * distance_to_horizon;
          wy = position.y + direction.y * distance_to_horizon;
          u = wx - std::floor(wx);
          v = wy - std::floor(wy);
          color = hit ? hit->ceiling->sample({ u, v }) : world::ceiling_default().sample({ u, v });
          light = std::clamp((distance_to_horizon + level) / max_d, -1.0f, 1.0f);
          color = lerp(color, normalized(world::palette().color(0)), light * light);
        }
        assign_pixel({ static_cast<std::int16_t>(i), static_cast<std::int16_t>(y) }, color);
        assign_depth({ static_cast<std::int16_t>(i), static_cast<std::int16_t>(y) }, 1.0f);
      }
      if (hit)
      {
        u = hit->side.x ? hit->position.y - std::floor(hit->position.y) : hit->position.x - std::floor(hit->position.x);
        v = (y + 0.5f - wall_top) / wall_height;
        dv = 1.0f / wall_height;
        light = std::clamp((hit->distance + level) / max_d, -1.0f, 1.0f);
        for (; y < static_cast<std::int32_t>(wall_bottom); ++y, v += dv)
        {
          color = hit->wall->sample({ u, v });
          color = lerp(color, normalized(world::palette().color(0)), light * light);
          assign_pixel({ static_cast<std::int16_t>(i), static_cast<std::int16_t>(y) }, color);
          assign_depth({ static_cast<std::int16_t>(i), static_cast<std::int16_t>(y) }, hit->distance / 20.0f);
        }
      }
      for (; y < m_info->extent->height; ++y)
      {
        {
          distance_to_horizon = 0.5f * m_info->extent->height / ((y + 0.5f) - m_info->extent->height * 0.5f);
          wx = position.x + direction.x * distance_to_horizon;
          wy = position.y + direction.y * distance_to_horizon;
          u = wx - std::floor(wx);
          v = wy - std::floor(wy);
          color = hit ? hit->floor->sample({ u, v }) : world::floor_default().sample({ u, v });
          light = std::clamp((distance_to_horizon + level) / max_d, -1.0f, 1.0f);
          color = lerp(color, normalized(world::palette().color(0)), light * light);
        }
        assign_pixel({ static_cast<std::int16_t>(i), static_cast<std::int16_t>(y) }, color);
        assign_depth({ static_cast<std::int16_t>(i), static_cast<std::int16_t>(y) }, 1.0f);
      }
    }
    {
      auto coords = offset_2d{ };
      auto i = std::size_t{ 0 };
      for (const auto& pickup : w.pickups())
      {
        ++i;
        coords.x = pickup.position().x;
        coords.y = pickup.position().y;
        if (!pickup.has_texture())
        {
          continue;
        }
        if (coords.x < 0 || coords.x >= w.extent().width ||
            coords.y < 0 || coords.y >= w.extent().height)
        {
          continue;
        }
        if (w.at(coords).pickup_id != i)
        {
          continue;
        }
        render(w, pickup);
      }
    }
    {
      auto coords = offset_2d{ };
     // auto i = std::size_t{ 0 };
      for (const auto& enemy : w.enemies())
      {
      //  ++i;
        coords.x = enemy.position().x;
        coords.y = enemy.position().y;
        if (!enemy.has_texture() || !enemy.hp())
        {
          continue;
        }
        if (coords.x < 0 || coords.x >= w.extent().width ||
            coords.y < 0 || coords.y >= w.extent().height)
        {
          continue;
        }
#if 0
        if (w.at(coords).solid_id != i)
        {
          continue;
        }
#endif
        render_enemy(w, enemy);
      }
    }
    if (w.player().was_hit() || !w.player().hp())
    {
      color = normalized(world::palette().color(3));
      color.w = 0.25f;
      filter(color);
    }
  }

  void frame::render(const sprite& spr) {
    const auto& position = spr.position();
    blit(spr.sheet(), spr.frame(),
         { { static_cast<std::int16_t>(position.x), static_cast<std::int16_t>(position.y) }, spr.frame().extent });
  }

  void frame::render(const world& w, const entity& ent) {
    const auto& texture = ent.texture();
    const auto& player = w.player();
    const auto max_d = std::sqrt(18.0f);
    auto delta = ent.position() - player.position();
    const auto fdist = dot(delta, player.forward());
    const auto rdist = dot(delta, player.right());
    if (fdist <= 0.01f || fdist > max_d)
    {
      return;
    }
    auto cx = rdist / (fdist * w.camera_plane());
    if (std::fabs(cx) > 1.0f)
    {
      return;
    }
    const auto level = 0.5f - 0.015f * (1.0f - std::cos(w.player_light_level()));
    const auto light = std::clamp((fdist + level) / max_d, -1.0f, 1.0f);
    const auto sx = static_cast<std::int16_t>(m_info->extent->width * (cx + 1.0f) * 0.5f);
    const auto depth = 1.0f / fdist;
    const auto height = m_info->extent->height * depth;
    const auto width = height * texture.extent().width / texture.extent().height;
    const auto top = m_info->extent->height * 0.5f - height * 0.5f;
    const auto start_y = std::max(static_cast<std::int16_t>(top), std::int16_t{ 0 });
    const auto end_y = std::min(static_cast<std::int16_t>(m_info->extent->height * 0.5f + height * 0.5f),
                                static_cast<std::int16_t>(m_info->extent->height));
    const auto left = sx - width * 0.5f;
    const auto aspect = vec2{ texture.extent().width / width, texture.extent().height / height };
    const auto start_x = std::max(static_cast<std::int16_t>(left), std::int16_t{ 0 });
    const auto end_x = std::min(static_cast<std::int16_t>(sx + width * 0.5f),
                                static_cast<std::int16_t>(m_info->extent->width));
    auto u = std::int16_t{ };
    auto v = std::int16_t{ };
    auto color = rgba{ };
    auto lit = vec4{ };
    auto index = std::size_t{ };
    for (int16_t i = start_x; i < end_x; ++i)
    {
      u = static_cast<std::int16_t>((i - left) * aspect.x);
      if (u < 0 || u >= 64)
      {
        continue;
      }
      for (int16_t j = start_y; j < end_y; ++j)
      {
        v = static_cast<std::int16_t>((j - top) * aspect.y);
        index = j * m_info->extent->width + i;
        if ((*m_info->depth)[index] < (fdist / 20.0f))
        {
          continue;
        }
        if (v < 0 || v >= 64)
        {
          continue;
        }
        color = texture.at({ u, v });
        if (!color.a)
        {
          continue;
        }
        lit = lerp(normalized(color), normalized(world::palette().color(0)), light * light);
        blend_pixel({ i, j }, lit, texture.has_premultiplied_alpha());
        (*m_info->depth)[index] = fdist / 20.0f;
      }
    }
  }

  void frame::render_enemy(const world& w, const entity& ent) {
    const auto& texture = ent.texture();
    const auto& player = w.player();
    const auto max_d = std::sqrt(18.0f);
    auto delta = ent.position() - player.position();
    const auto fdist = dot(delta, player.forward());
    const auto rdist = dot(delta, player.right());
    if (fdist <= 0.01f || fdist > max_d)
    {
      return;
    }
    auto cx = rdist / (fdist * w.camera_plane());
    if (std::fabs(cx) > 1.0f)
    {
      return;
    }

    const auto front_or_back = dot(ent.forward(), -delta);
    auto base_u = std::int16_t{ };
    if (front_or_back > 0)
    {
      base_u = 0;
    }
    else if (front_or_back <= 0)
    {
      base_u = 64;
    }
    const auto level = 0.5f - 0.015f * (1.0f - std::cos(w.player_light_level()));
    const auto light = std::clamp((fdist + level) / max_d, -1.0f, 1.0f);
    const auto sx = static_cast<std::int16_t>(m_info->extent->width * (cx + 1.0f) * 0.5f);
    const auto depth = 1.0f / fdist;
    const auto height = m_info->extent->height * depth;
    const auto width = height;
    const auto top = m_info->extent->height * 0.5f - height * 0.5f;
    const auto start_y = std::max(static_cast<std::int16_t>(top), std::int16_t{ 0 });
    const auto end_y = std::min(static_cast<std::int16_t>(m_info->extent->height * 0.5f + height * 0.5f),
                                static_cast<std::int16_t>(m_info->extent->height));
    const auto left = sx - width * 0.5f;
    const auto aspect = vec2{ 64.0f / width, 64.0f / height };
    const auto start_x = std::max(static_cast<std::int16_t>(left), std::int16_t{ 0 });
    const auto end_x = std::min(static_cast<std::int16_t>(sx + width * 0.5f),
                                static_cast<std::int16_t>(m_info->extent->width));
    auto u = std::int16_t{ };
    auto v = std::int16_t{ };
    auto color = rgba{ };
    auto lit = vec4{ };
    auto index = std::size_t{ };
    for (int16_t i = start_x; i < end_x; ++i)
    {
      u = static_cast<std::int16_t>((i - left) * aspect.x + base_u);
      if (u < 0 || u >= base_u + 64)
      {
        continue;
      }
      for (int16_t j = start_y; j < end_y; ++j)
      {
        v = static_cast<std::int16_t>((j - top) * aspect.y);
        index = j * m_info->extent->width + i;
        if ((*m_info->depth)[index] < (fdist / 20.0f))
        {
          continue;
        }
        if (v < 0 || v >= 64)
        {
          continue;
        }
        color = texture.at({ u, v });
        if (!color.a)
        {
          continue;
        }
        lit = lerp(normalized(color), normalized(world::palette().color(0)), light * light);
        blend_pixel({ i, j }, lit, texture.has_premultiplied_alpha());
        (*m_info->depth)[index] = fdist / 20.0f;
      }
    }
  }

  struct renderer::info final {
    std::array<std::vector<rgba>, FRAMES_IN_FLIGHT> cpus;
    std::array<std::vector<float>, FRAMES_IN_FLIGHT> depths;
    std::array<GLuint, FRAMES_IN_FLIGHT> stagings;
    std::array<GLuint, FRAMES_IN_FLIGHT> pbos;
    std::array<GLuint, FRAMES_IN_FLIGHT> attachments;
    std::array<GLuint, FRAMES_IN_FLIGHT> framebuffers;
    std::array<void*, FRAMES_IN_FLIGHT> mappings;
    std::array<GLsync, FRAMES_IN_FLIGHT> fences;
    std::array<frame::info, FRAMES_IN_FLIGHT> frame_infos;
    extent_2d extent;
    // Undefined to render without this bound even if I don't use it!
    GLuint default_vertex_array;
    GLuint srgb_program;
    std::size_t frame_id;
  };

  static const char *const srgb_vertex_shader =
    "#version 450 core\n"
    "const vec4 vertices[3] = vec4[3](vec4(-1.0, -1.0, 0.0, 1.0),\n"
    "                                 vec4(3.0, -1.0, 0.0, 1.0),\n"
    "                                 vec4(-1.0, 3.0, 0.0, 1.0));\n"
    "void main() {\n"
    "  gl_Position = vertices[gl_VertexID];\n"
    "}\n";

  static const char *const srgb_fragment_shader =
    "#version 450 core\n"
    "layout (location = 0) uniform sampler2D u_linear;\n"
    "layout (location = 1) uniform uvec2 u_framebuffer_size;\n"
    "layout (location = 0) out vec4 color_0;\n"
    "float gamma(const float channel) {\n"
    "  return float(channel <= 0.0031308) * (channel * 12.92) +\n"
    "         float(channel > 0.0031308) * (1.055 * pow(channel, 1.0 / 2.4) - 0.055);\n"
    "}\n"
    "vec4 gamma(const vec4 color) {\n"
    "  return vec4(gamma(color.r), gamma(color.g), gamma(color.b), color.a);\n"
    "}\n"
    "void main() {\n"
    "  vec2 uv = gl_FragCoord.xy / vec2(u_framebuffer_size);\n"
    "  uv.y = 1.0 - uv.y;\n"
    "  color_0 = gamma(texture(u_linear, uv));\n"
    "}\n";

  renderer::renderer(std::unique_ptr<sdl3::window>&& parent, const extent_2d& extent) :
  m_parent{ std::move(parent) },
  m_info{ new info{ } } {
    assert(m_info != nullptr);
    if (!m_parent->is_current())
    {
      throw error{ "The window associated with the requested renderer is not the current OpenGL context." };
    }
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glCreateVertexArrays(1, &m_info->default_vertex_array);
    const auto vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &srgb_vertex_shader, nullptr);
    glCompileShader(vertex);
    const auto fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &srgb_fragment_shader, nullptr);
    glCompileShader(fragment);
    m_info->srgb_program = glCreateProgram();
    glAttachShader(m_info->srgb_program, vertex);
    glAttachShader(m_info->srgb_program, fragment);
    glLinkProgram(m_info->srgb_program);
    glDeleteShader(fragment);
    glDeleteShader(vertex);
    apply_viewport();
    resize(extent);
  }

  renderer::~renderer() noexcept {
    glFlush();
    for (auto& fence : m_info->fences)
    {
      if (fence)
      {
        glClientWaitSync(fence, 0, std::numeric_limits<GLuint64>::max());
        glDeleteSync(fence);
      }
    }
    for (auto& staging : m_info->stagings)
    {
      glUnmapNamedBuffer(staging);
    }
    glDeleteFramebuffers(FRAMES_IN_FLIGHT, m_info->framebuffers.data());
    glDeleteTextures(FRAMES_IN_FLIGHT, m_info->attachments.data());
    glDeleteBuffers(FRAMES_IN_FLIGHT, m_info->stagings.data());
    glDeleteProgram(m_info->srgb_program);
    glDeleteVertexArrays(1, &m_info->default_vertex_array);
  }

  void renderer::resize(const extent_2d& extent) {
    assert(m_parent->is_current());
    // Wait for all frames to be ready!
    glFlush();
    for (auto& fence : m_info->fences)
    {
      if (fence)
      {
        glClientWaitSync(fence, 0, std::numeric_limits<GLuint64>::max());
        glDeleteSync(fence);
        // I'm pretty sure OpenGL says GLsync is a pointer, but it doesn't expose that it is. C'est la vie.
        fence = 0;
      }
    }
    for (auto i = std::size_t{ 0 }; i < FRAMES_IN_FLIGHT; ++i)
    {
      if (m_info->mappings[i])
      {
        glUnmapNamedBuffer(m_info->stagings[i]);
        m_info->mappings[i] = nullptr;
      }
    }
    for (auto& cpu : m_info->cpus)
    {
      cpu.clear();
    }
    glDeleteBuffers(FRAMES_IN_FLIGHT, m_info->stagings.data());
    m_info->stagings.fill(0);
    glDeleteBuffers(FRAMES_IN_FLIGHT, m_info->pbos.data());
    m_info->pbos.fill(0);
    glDeleteTextures(FRAMES_IN_FLIGHT, m_info->attachments.data());
    m_info->attachments.fill(0);
    glDeleteFramebuffers(FRAMES_IN_FLIGHT, m_info->framebuffers.data());
    m_info->framebuffers.fill(0);
    m_info->extent = extent;
    if (!m_info->extent.width || !m_info->extent.height)
    {
      return;
    }
    // Cast to ensure full 16-bit range of the extent is valid, so we're not multiplying 65535 * 65535 as shorts but
    // as GLsizei (probably int64_t).
    const auto pixels = static_cast<GLsizei>(m_info->extent.width) * static_cast<GLsizei>(m_info->extent.height);
    for (auto& cpu : m_info->cpus)
    {
      cpu.resize(pixels);
    }
    for (auto& depth : m_info->depths)
    {
      depth.resize(pixels);
    }
    glCreateBuffers(FRAMES_IN_FLIGHT, m_info->stagings.data());
    glCreateBuffers(FRAMES_IN_FLIGHT, m_info->pbos.data());
    glCreateTextures(GL_TEXTURE_2D, FRAMES_IN_FLIGHT, m_info->attachments.data());
    glCreateFramebuffers(FRAMES_IN_FLIGHT, m_info->framebuffers.data());
    for (auto i = std::size_t{ 0 }; i < FRAMES_IN_FLIGHT; ++i)
    {
      glNamedBufferStorage(m_info->stagings[i], pixels * sizeof(rgba), nullptr,
                           GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
      glNamedBufferStorage(m_info->pbos[i], pixels * sizeof(rgba), nullptr, 0);
      auto *const res = glMapNamedBufferRange(m_info->stagings[i], 0, pixels * sizeof(rgba),
                                              GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT);
      if (!res)
      {
        throw error{ "The renderer failed to map a staging buffer." };
      }
      m_info->mappings[i] = res;
      glTextureStorage2D(m_info->attachments[i], 1, GL_RGBA8, extent.width, extent.height);
      glTextureParameteri(m_info->attachments[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glNamedFramebufferTexture(m_info->framebuffers[i], GL_COLOR_ATTACHMENT0, m_info->attachments[i], 0);
    }
    glFlush();
  }

  void renderer::apply_viewport() {
    const auto frame = m_parent->viewport();
    apply_viewport({ { 0, 0 }, frame.extent });
  }

  void renderer::apply_viewport(const rect_2d& viewport) {
    assert(m_parent->is_current());
    glFlush();
    glViewport(viewport.offset.x, viewport.offset.y, viewport.extent.width, viewport.extent.height);
    glFlush();
  }

  sdl3::window& renderer::window() {
    return *m_parent;
  }

  const sdl3::window& renderer::window() const {
    return *m_parent;
  }

  frame renderer::begin_rendering() {
    const auto i = m_info->frame_id & (FRAMES_IN_FLIGHT - 1);
    if (m_info->fences[i])
    {
      glClientWaitSync(m_info->fences[i], GL_SYNC_FLUSH_COMMANDS_BIT, std::numeric_limits<GLuint64>::max());
      glDeleteSync(m_info->fences[i]);
      m_info->fences[i] = 0;
    }
    glClear(GL_COLOR_BUFFER_BIT);
    m_info->frame_infos[i].pixels = &m_info->cpus[i];
    m_info->frame_infos[i].depth = &m_info->depths[i];
    m_info->frame_infos[i].frame_id = m_info->frame_id++;
    m_info->frame_infos[i].extent = &m_info->extent;
    return frame{ m_info->frame_infos[i] };
  }

  void renderer::end_rendering(frame&& finished) {
    const auto i = finished.m_info->frame_id & (FRAMES_IN_FLIGHT - 1);
    std::memcpy(m_info->mappings[i], m_info->cpus[i].data(), m_info->cpus[i].size() * sizeof(rgba));
    glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
    glCopyNamedBufferSubData(m_info->stagings[i], m_info->pbos[i], 0, 0, m_info->cpus[i].size() * sizeof(rgba));
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_info->pbos[i]);
    glTextureSubImage2D(m_info->attachments[i], 0, 0, 0, m_info->extent.width, m_info->extent.height, GL_RGBA,
                        GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    const auto& viewport = m_parent->viewport();
    if (m_parent->supports_srgb())
    {
      glBlitNamedFramebuffer(m_info->framebuffers[i], 0,
                             0, m_info->extent.height, m_info->extent.width, 0,
                             0, 0, viewport.extent.width, viewport.extent.height,
                             GL_COLOR_BUFFER_BIT, GL_NEAREST);
      m_info->fences[i] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
    else
    {
      glBindVertexArray(m_info->default_vertex_array);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, m_info->attachments[i]);
      glUseProgram(m_info->srgb_program);
      glUniform1i(0, 0);
      glUniform2ui(1, viewport.extent.width, viewport.extent.height);
      glDrawArrays(GL_TRIANGLES, 0, 3);

    }
    while (glGetError() != GL_NO_ERROR)
    {
      throw error{ "An unspecified OpenGL error occurred." };
    }
  }

  std::unique_ptr<sdl3::window> release(std::unique_ptr<renderer>&& render) {
    if (render)
    {
      auto dead = std::unique_ptr<renderer>{ std::move(render) };
      auto window = std::move(dead->m_parent);
      return window;
    }
    return nullptr;
  }
}
