#ifndef RATS_WORLD_HPP
#define RATS_WORLD_HPP

#include <cinttypes>

#include <ranges>
#include <type_traits>
#include <vector>
#include <optional>
#include <queue>

#ifdef _MSC_VER
#  define __attribute__(exp)
#endif
#include <pcg_random.hpp>
#ifdef _MSC_VER
#  undef __attribute__
#endif

#include "geometry.hpp"
#include "math.hpp"
#include "assets.hpp"
#include "entity.hpp"
#include "easing.hpp"
#include "stopwatch.hpp"

namespace rats::sdl3 {

  class mixer;

}

namespace rats {

  struct wall_query final {
    vec2 start;
    vec2 direction;
    float max_distance;
  };

  struct wall_hit final {
    vec2 position;
    offset_2d side;
    float distance;
    assets::texture* wall;
    assets::texture* ceiling;
    assets::texture* floor;
  };

  class world final {
  public:
    constexpr static const std::uint8_t CELL_FLAG_NONE_BIT{ 0 };
    constexpr static const std::uint8_t CELL_FLAG_SOLID_BIT{ 1 };
    constexpr static const std::uint8_t CELL_FLAG_SEEN_BIT{ 2 };
    constexpr static const std::uint8_t CELL_FLAG_PICKUP_BIT{ 4 };
    constexpr static const std::uint8_t CELL_FLAG_NPC_BIT{ 8 };

    struct cell final {
      std::uint8_t wall;
      std::uint8_t ceiling;
      std::uint8_t floor;
      std::uint8_t flags;
      std::uint16_t solid_id;
      std::uint16_t pickup_id;
    };
  private:
    struct navigation_node final {
      offset_2d coordinates;
      offset_2d origin;
    };

    static std::unique_ptr<assets::texture> sm_ceiling_and_floor;
    static std::unique_ptr<assets::texture> sm_walls;
    static std::unique_ptr<assets::sound> sm_pickup_sfx;
    static std::unique_ptr<assets::sound> sm_bump_sfx;
    static std::unique_ptr<assets::sound> sm_attack_sfx;
    static std::unique_ptr<assets::sound> sm_hit_sfx;
    static std::unique_ptr<assets::sound> sm_die_sfx;
    static std::unique_ptr<assets::sound> sm_wakeup_sfx;
    static const assets::palette_2bit sm_palette;

    extent_2d m_extent;
    offset_2d m_start;
    mutable std::vector<cell> m_cells;
    std::vector<entity> m_entities;
    std::vector<entity> m_pickups;
    entity m_player_entity;
    float m_camera_plane;
    float m_player_light_level;
    bool m_automap;
    stopwatch::duration_type m_time_since_last_turn;
    mutable std::queue<navigation_node> m_navigator;
    mutable std::vector<std::uint8_t> m_navigator_explored;


    world(const extent_2d& extent, const offset_2d& start);
  public:
    static const assets::palette_2bit& palette();
    static const assets::texture& ceiling_default();
    static const assets::texture& wall_default();
    static const assets::texture& floor_default();
    static const assets::sound& pickup_sfx_default();
    static const assets::sound& bump_sfx_default();
    static const assets::sound& attack_sfx_default();
    static const assets::sound& hit_sfx_default();
    static const assets::sound& die_sfx_default();
    static const assets::sound& wakeup_sfx_default();

    template <std::ranges::forward_range Range>
    requires std::is_same_v<std::ranges::range_value_t<Range>, world::cell>
    world(const Range& r, const extent_2d& extent, const offset_2d& start, const std::vector<entity>& pickups,
          const std::vector<entity>& enemies) :
    world{ extent, start } {
      auto i = std::uint32_t{ 0 };
      for (auto cur = std::ranges::begin(r); cur != std::ranges::end(r) && i < m_cells.size(); ++cur, ++i)
      {
        m_cells[i] = *cur;
      }
      m_pickups = pickups;
      m_entities = enemies;
    }
    world(const world& other) = delete;
    world(world&& other) = delete;

    ~world() noexcept = default;

    world& operator=(const world& rhs) = delete;
    world& operator=(world&& rhs) = delete;

    const std::vector<entity>& entities() const;
    void add_player(const entity& ent);
    bool walk_player_to(const vec2& position, sdl3::mixer& mixer);
    bool have_player_attack(sdl3::mixer& mixer);
    void rotate_player_by(const float theta);
    const entity& player() const;
    entity& player();
    float camera_plane() const;
    void update(const stopwatch::duration_type& dt, sdl3::mixer& mixer);
    float player_light_level() const;

    bool is_showing_automap() const;
    void toggle_automap();
    const extent_2d& extent() const;
    const cell& at(const offset_2d& coords) const;
    const std::vector<entity>& pickups() const;
    const std::vector<entity>& enemies() const;
    const offset_2d& start() const;
    std::optional<wall_hit> wallcast(const wall_query& query) const;
    bool is_occupied(const offset_2d& cell) const;
    std::optional<vec2> navigate(const vec2& from, const vec2& to) const;
  };

  class world_generator final {
  public:
    using state_type = pcg32::state_type;
  private:
    pcg32::state_type m_rng_seed;
    pcg32::state_type m_rng_stream;
    pcg32 m_rng;
  public:
    world_generator();
    world_generator(const state_type seed, const state_type stream, const state_type position);
    world_generator(const world_generator& other);
    world_generator(world_generator&& other);

    ~world_generator() noexcept = default;

    world_generator& operator=(const world_generator& rhs);
    world_generator& operator=(world_generator&& rhs);

    world generate(const extent_2d& extent);
  };

}

#endif
