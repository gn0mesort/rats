#ifndef RATS_ASSETS_HPP
#define RATS_ASSETS_HPP

#include <filesystem>
#include <optional>
#include <vector>
#include <unordered_map>
#include <array>
#include <ranges>
#include <type_traits>

#include "math.hpp"
#include "geometry.hpp"
#include "graphics.hpp"

namespace rats::assets {

  std::filesystem::path executable_directory();
  std::filesystem::path home_directory();
  std::optional<std::filesystem::path> find_asset(const std::filesystem::path& path);

  class texture final {
  private:
    std::vector<rgba> m_texels;
    extent_2d m_extent;
    bool m_premultiplied;
  public:
    explicit texture(const extent_2d& extent);
    explicit texture(const std::filesystem::path& path);
    texture(const std::filesystem::path& path, const bool linear);
    texture(const texture& other) = default;

    ~texture() noexcept = default;

    texture& operator=(const texture& rhs) = default;
    const rgba& operator[](const offset_2d& coordinates) const;
    rgba& operator[](const offset_2d& coordinates);

    vec4 sample(const vec2& coordinates) const;
    const rgba& at(const offset_2d& coordinates) const;
    rgba& at(const offset_2d& coordinates);

    bool has_premultiplied_alpha() const;
    void premultiply_alpha();

    const extent_2d& extent() const;
  };

  class palette_2bit final {
    std::array<rgba, 4> m_colors;
  public:
    template <std::ranges::forward_range ForwardRange>
    requires std::is_same_v<std::ranges::range_value_t<ForwardRange>, rgba>
    explicit palette_2bit(ForwardRange&& r, const bool linear = false) {
      auto cur = std::begin(r);
      for (auto& color : m_colors)
      {
        color = *cur;
        if (!linear)
        {
          color = denormalized(to_linear(normalized(color)));
        }
        std::advance(cur, 1);
      }
    }
    palette_2bit(const palette_2bit& other) = default;
    palette_2bit(palette_2bit&& other) = default;

    ~palette_2bit() noexcept = default;

    palette_2bit& operator=(const palette_2bit& rhs) = default;
    palette_2bit& operator=(palette_2bit&& rhs) = default;

    const rgba& color(const std::size_t index) const;
    texture apply_to(const texture& base) const;
  };

  class sound final {
  private:
    std::vector<float> m_samples;
    std::size_t m_frame_count;
  public:
    explicit sound(const std::filesystem::path& path);
    sound(const sound& other) = default;
    sound(sound&& other);

    ~sound() noexcept = default;

    sound& operator=(const sound& rhs) = default;
    sound& operator=(sound&& rhs);

    std::size_t frame_count() const;
    const std::vector<float>& samples() const;
  };

}

#endif
