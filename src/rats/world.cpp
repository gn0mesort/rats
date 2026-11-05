#include "rats/world.hpp"

#include <cstddef>

#include <algorithm>
#include <random>

#include "rats/math.hpp"
#include "rats/error.hpp"

#include "rats/sdl3.hpp"

namespace {

  struct wall_candidate final {
    rats::offset_2d origin;
    rats::offset_2d wall;
  };

  void adjacent_walls(const rats::offset_2d& cell, const rats::extent_2d& extent, std::vector<wall_candidate>& walls) {
    if (cell.x + 2 < extent.width - 1)
    {
      walls.emplace_back(wall_candidate{ cell, { static_cast<std::int16_t>(cell.x + 1), cell.y } });
    }
    if (cell.x - 2 > 0)
    {
      walls.emplace_back(wall_candidate{ cell, { static_cast<std::int16_t>(cell.x - 1), cell.y } });
    }
    if (cell.y + 2 < extent.height - 1)
    {
      walls.emplace_back(wall_candidate{ cell, { cell.x, static_cast<std::int16_t>(cell.y + 1) } });
    }
    if (cell.y - 2 > 0)
    {
      walls.emplace_back(wall_candidate{ cell, { cell.x, static_cast<std::int16_t>(cell.y - 1) } });
    }
  }

}

namespace rats {

  world_generator::world_generator() :
  world_generator{ std::random_device{ }(), std::random_device{ }(), 0 } { }

  world_generator::world_generator(const state_type seed, const state_type stream, const state_type position) :
  m_rng_seed{ seed },
  m_rng_stream{ stream },
  m_rng{ seed, stream } {
    m_rng.discard(position);
  }

  world_generator::world_generator(const world_generator& other) :
  m_rng_seed{ other.m_rng_seed },
  m_rng_stream{ other.m_rng_stream },
  m_rng{ m_rng_seed, m_rng_stream } {
    const auto position = other.m_rng - m_rng;
    m_rng.discard(position);
  }

  world_generator::world_generator(world_generator&& other) :
  m_rng_seed{ std::move(other.m_rng_seed) },
  m_rng_stream{ std::move(other.m_rng_stream) },
  m_rng{ std::move(other.m_rng) } { }

  world_generator& world_generator::operator=(const world_generator& rhs) {
    if (this != &rhs)
    {
      auto cpy = world_generator{ rhs };
      std::swap(m_rng_seed, cpy.m_rng_seed);
      std::swap(m_rng_stream, cpy.m_rng_stream);
      std::swap(m_rng, cpy.m_rng);
    }
    return *this;
  }

  world_generator& world_generator::operator=(world_generator&& rhs) {
    if (this != &rhs)
    {
      m_rng_seed = std::move(rhs.m_rng_seed);
      m_rng_stream = std::move(rhs.m_rng_stream);
      m_rng = std::move(rhs.m_rng);
    }
    return *this;
  }

  world world_generator::generate(const extent_2d& extent) {
    auto buffer = std::vector<world::cell>(extent.width * extent.height);
    std::ranges::fill(buffer, world::cell{ 1, 1, 1, world::CELL_FLAG_SOLID_BIT, 0, 0 });
    auto random_x = std::uniform_int_distribution<std::size_t>{ 1, static_cast<std::size_t>(extent.width - 2) };
    auto random_y = std::uniform_int_distribution<std::size_t>{ 1, static_cast<std::size_t>(extent.height - 2) };
    auto x = random_x(m_rng);
    auto y = random_y(m_rng);
    auto index = y * extent.width + x;
    buffer[index].wall = 0;
    buffer[index].flags = world::CELL_FLAG_NONE_BIT;
    auto start = offset_2d{ static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) };
    auto walls = std::vector<wall_candidate>{ };
    adjacent_walls(start, extent, walls);
    auto random_choice = std::uniform_int_distribution<std::size_t>{ };
    auto choice = std::size_t{ 0 };
    auto direction = offset_2d{ };
    auto candidate = wall_candidate{ };
    while (!walls.empty())
    {
      random_choice = std::uniform_int_distribution<std::size_t>{ 0, walls.size() - 1 };
      choice = random_choice(m_rng);
      candidate = walls[choice];
      std::swap(walls[choice], walls.back());
      walls.pop_back();
      direction = { static_cast<std::int16_t>(candidate.wall.x - candidate.origin.x),
                    static_cast<std::int16_t>(candidate.wall.y - candidate.origin.y) };
      x = candidate.wall.x + direction.x;
      y = candidate.wall.y + direction.y;
      index = y * extent.width + x;
      if (buffer[index].wall != 0)
      {
        buffer[index].wall = 0;
        buffer[index].flags = world::CELL_FLAG_NONE_BIT;
        buffer[candidate.wall.y * extent.width + candidate.wall.x].wall = 0;
        buffer[candidate.wall.y * extent.width + candidate.wall.x].flags = world::CELL_FLAG_NONE_BIT;
        adjacent_walls({ static_cast<std::int16_t>(x), static_cast<std::int16_t>(y) }, extent, walls);
      }
    }
    auto remaining_room_area = static_cast<std::int64_t>(0.1f * buffer.size());
    auto random_w = std::uniform_int_distribution<std::size_t>{ };
    auto random_h = std::uniform_int_distribution<std::size_t>{ };
    auto max_side = std::size_t{ };
    auto w = std::size_t{ };
    auto h = std::size_t{ };
    while (remaining_room_area > 0)
    {
      max_side = std::min(static_cast<std::size_t>(std::sqrt(remaining_room_area)), std::size_t{ 4 });
      random_w = std::uniform_int_distribution<std::size_t>{ 2, max_side + 1 };
      random_h = std::uniform_int_distribution<std::size_t>{ 2, max_side + 1 };
      w = random_w(m_rng);
      x = std::min(random_x(m_rng), extent.width - w - 2);
      h = random_h(m_rng);
      y = std::min(random_y(m_rng), extent.height - h - 2);
      for (auto i = std::size_t{ 0 }; i < h; ++i)
      {
        for (auto j = std::size_t{ 0 }; j < w; ++j)
        {
          index = (y + i) * extent.width + x + j;
          buffer[index].wall = 0;
          buffer[index].flags = world::CELL_FLAG_NONE_BIT;
          --remaining_room_area;
        }
      }
    }

    auto pickup_textures = std::array<std::shared_ptr<assets::texture>, 4>{ };
    {
      const auto paths = std::array<std::filesystem::path, 4>{
        *assets::find_asset("assets/img/pie.webp"),
        *assets::find_asset("assets/img/cheese.webp"),
        *assets::find_asset("assets/img/coins.webp"),
        *assets::find_asset("assets/img/ruby.webp")
      };
      auto cur = pickup_textures.begin();
      for (const auto& path : paths)
      {
        const auto texture = world::palette().apply_to(assets::texture{ path, true });
        (*cur).reset(new assets::texture{ texture });
        (*cur)->premultiply_alpha();
        ++cur;
      }
    }
    auto enemy_texture = std::shared_ptr<assets::texture>{ };
    {
      const auto texture = world::palette().apply_to(assets::texture{ *assets::find_asset("assets/img/bishop.webp"),
                                                                      true });
      enemy_texture.reset(new assets::texture{ texture });
      enemy_texture->premultiply_alpha();
    }
    auto pickups = std::vector<entity>{ };
    auto pickup_chance = std::uniform_int_distribution<std::size_t>{ 0, 999 };
    auto pickup_kind = std::uniform_int_distribution<std::size_t>{ 0, 3 };
    auto enemies = std::vector<entity>{ };
    auto enemy_chance = std::uniform_int_distribution<std::size_t>{ 0, 999 };
    auto ent = entity{ { 0, 0 } };
    auto next = vec2{ };
    auto rotation = float{ };
    auto type = entity_type::unknown;
    auto kind = 0;
    auto amount = 0;
    for (auto i = std::size_t{ 0 }; i < extent.height; ++i)
    {
      for (auto j = std::size_t{ 0 }; j < extent.width; ++j)
      {
        if (j == static_cast<std::size_t>(start.x) && i == static_cast<std::size_t>(start.y))
        {
          continue;
        }
        index = i * extent.width + j;
        if (!(buffer[index].flags & world::CELL_FLAG_SOLID_BIT))
        {
          if (pickup_chance(m_rng) >= 750)
          {
            amount = 0;
            kind = pickup_kind(m_rng);
            if (kind < 2)
            {
              type = entity_type::food;
              switch (kind)
              {
              case 0:
                amount = 15;
                break;
              case 1:
                amount = 5;
                break;
              default:
                break;
              }
            }
            else
            {
              type = entity_type::treasure;
              switch (kind)
              {
              case 2:
                amount = 10;
                break;
              case 3:
                amount = 100;
                break;
              default:
                break;
              }
            }
            ent = entity{ { j + 0.5f, i + 0.5f }, 0.0f, pickup_textures[kind], type };
            ent.set_hp_to(amount);
            pickups.push_back(ent);
            buffer[index].pickup_id = pickups.size();
            buffer[index].flags |= world::CELL_FLAG_PICKUP_BIT;
          }
          if (enemy_chance(m_rng) >= 850)
          {
            ent = entity{ { j + 0.5f, i + 0.5f }, 0.0f, enemy_texture, entity_type::enemy };
            next = ent.position() + ent.forward();
            rotation = 0.0f;
            while (buffer[next.y * extent.width + next.x].flags & world::CELL_FLAG_SOLID_BIT &&
                   rotation <= tau_v<float>)
            {
              rotation += tau_v<float> / 8.0f;
              ent.rotate_by(tau_v<float> / 8.0f);
              next = ent.position() + ent.forward();
            }
            enemies.push_back(ent);
            buffer[index].solid_id = enemies.size();
            buffer[index].flags |= world::CELL_FLAG_NPC_BIT;
          }
        }
      }
    }
    return world{ buffer, extent, start, pickups, enemies };
  }

  const assets::palette_2bit world::sm_palette{ std::array<rgba, 4>{ rgba{ 0x1a, 0x21, 0x29, 0xff },
                                                                     rgba{ 0x31, 0x21, 0x37, 0xff },
                                                                     rgba{ 0x51, 0x28, 0x39, 0xff },
                                                                     rgba{ 0x71, 0x31, 0x41, 0xff } } };
  std::unique_ptr<assets::texture> world::sm_ceiling_and_floor{ };
  std::unique_ptr<assets::texture> world::sm_walls{ };
  std::unique_ptr<assets::sound> world::sm_pickup_sfx{ };
  std::unique_ptr<assets::sound> world::sm_bump_sfx{ };
  std::unique_ptr<assets::sound> world::sm_attack_sfx{ };
  std::unique_ptr<assets::sound> world::sm_hit_sfx{ };
  std::unique_ptr<assets::sound> world::sm_die_sfx{ };
  std::unique_ptr<assets::sound> world::sm_wakeup_sfx{ };

  const assets::palette_2bit& world::palette() {
    return sm_palette;
  }

  const assets::texture& world::ceiling_default() {
    if (!sm_ceiling_and_floor)
    {
      const auto path = assets::find_asset("assets/img/stonefloor.webp");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      const auto asset = assets::texture{ *path, true };
      sm_ceiling_and_floor.reset(new assets::texture{ sm_palette.apply_to(asset) });
    }
    return *sm_ceiling_and_floor;
  }

  const assets::texture& world::wall_default() {
    if (!sm_walls)
    {
      const auto path = assets::find_asset("assets/img/cavewall.webp");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      const auto asset = assets::texture{ *path, true };
      sm_walls.reset(new assets::texture{ sm_palette.apply_to(asset) });
    }
    return *sm_walls;
  }

  const assets::texture& world::floor_default() {
    if (!sm_ceiling_and_floor)
    {
      const auto path = assets::find_asset("assets/img/stonefloor.webp");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      const auto asset = assets::texture{ *path, true };
      sm_ceiling_and_floor.reset(new assets::texture{ sm_palette.apply_to(asset) });
    }
    return *sm_ceiling_and_floor;
  }

  const assets::sound& world::pickup_sfx_default() {
    if (!sm_pickup_sfx)
    {
      const auto path = assets::find_asset("assets/snd/pickup.mp3");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      sm_pickup_sfx.reset(new assets::sound{ *path });
    }
    return *sm_pickup_sfx;
  }

  const assets::sound& world::bump_sfx_default() {
    if (!sm_bump_sfx)
    {
      const auto path = assets::find_asset("assets/snd/bump.mp3");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      sm_bump_sfx.reset(new assets::sound{ *path });
    }
    return *sm_bump_sfx;

  }

  const assets::sound& world::attack_sfx_default() {
    if (!sm_attack_sfx)
    {
      const auto path = assets::find_asset("assets/snd/attack.mp3");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      sm_attack_sfx.reset(new assets::sound{ *path });
    }
    return *sm_attack_sfx;
  }

  const assets::sound& world::hit_sfx_default() {
    if (!sm_hit_sfx)
    {
      const auto path = assets::find_asset("assets/snd/ouch.mp3");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      sm_hit_sfx.reset(new assets::sound{ *path });
    }
    return *sm_hit_sfx;
  }

  const assets::sound& world::die_sfx_default() {
    if (!sm_die_sfx)
    {
      const auto path = assets::find_asset("assets/snd/die.mp3");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      sm_die_sfx.reset(new assets::sound{ *path });
    }
    return *sm_die_sfx;
  }

  const assets::sound& world::wakeup_sfx_default() {
    if (!sm_wakeup_sfx)
    {
      const auto path = assets::find_asset("assets/snd/wakeup.mp3");
      if (!path)
      {
        throw error{ error::origin::application, "The application failed to find a required file." };
      }
      sm_wakeup_sfx.reset(new assets::sound{ *path });
    }
    return *sm_wakeup_sfx;
  }

  world::world(const extent_2d& extent, const offset_2d& start) :
  m_extent{ extent },
  m_start{ start },
  m_cells(static_cast<std::size_t>(m_extent.width) * static_cast<std::size_t>(m_extent.height)),
  m_player_light_level{ 0.0f },
  m_automap{ false },
  m_time_since_last_turn{ stopwatch::duration_type::zero() },
  m_navigator{ },
  m_navigator_explored(m_cells.size()) {
    assert(m_extent.width > 0);
    assert(m_extent.height > 0);
    assert(m_start.x >= 0 && m_start.x < m_extent.width);
    assert(m_start.y >= 0 && m_start.y < m_extent.height);
    world::ceiling_default();
    world::wall_default();
    world::floor_default();
    m_camera_plane = std::tan(radians(35.0f));
  }

  const offset_2d& world::start() const {
    return m_start;
  }

  std::optional<wall_hit> world::wallcast(const wall_query& query) const {
    auto res = wall_hit{ };
    auto cx = static_cast<std::int16_t>(query.start.x);
    auto cy = static_cast<std::int16_t>(query.start.y);
    const auto sx = static_cast<std::int16_t>((query.direction.x < 0.0f) ? -1 : 1);
    const auto sy = static_cast<std::int16_t>((query.direction.y < 0.0f) ? -1 : 1);
    const auto dx = is_almost_zero(query.direction.x, EPSILON) ? INFINITY : std::fabs(1.0f / query.direction.x);
    const auto dy = is_almost_zero(query.direction.y, EPSILON) ? INFINITY : std::fabs(1.0f / query.direction.y);
    auto next_v_grid = 0.0f;
    if (query.direction.x < 0.0f)
    {
      next_v_grid = (query.start.x - cx) * dx;
    }
    else
    {
      next_v_grid = (cx + 1.0f - query.start.x) * dx;
    }
    auto next_h_grid = 0.0f;
    if (query.direction.y < 0.0f)
    {
      next_h_grid = (query.start.y - cy) * dy;
    }
    else
    {
      next_h_grid = (cy + 1.0f - query.start.y) * dy;
    }
    auto current = std::size_t{ 0 };
    while (std::min(next_v_grid, next_h_grid) <= query.max_distance)
    {
      if (next_v_grid < next_h_grid)
      {
        cx += sx;
        if (cx < 0 || cx >= m_extent.width || cy < 0 || cy >= m_extent.height)
        {
          break;
        }
        current = cy * m_extent.width + cx;
        m_cells[current].flags |= CELL_FLAG_SEEN_BIT;
        if (m_cells[current].wall)
        {
          res.side = { sx, 0 };
          res.distance = next_v_grid;
          res.ceiling = sm_ceiling_and_floor.get();
          res.wall = sm_walls.get();
          res.floor = sm_ceiling_and_floor.get();
          res.position = { query.start.x + query.direction.x * res.distance,
                          query.start.y + query.direction.y * res.distance };
          return res;
        }
        next_v_grid += dx;
      }
      else
      {
        cy += sy;
        if (cx < 0 || cx >= m_extent.width || cy < 0 || cy >= m_extent.height)
        {
          break;
        }
        current = cy * m_extent.width + cx;
        m_cells[current].flags |= CELL_FLAG_SEEN_BIT;
        if (m_cells[current].wall)
        {
          res.side = { 0, sy };
          res.distance = next_h_grid;
          res.ceiling = sm_ceiling_and_floor.get();
          res.wall = sm_walls.get();
          res.floor = sm_ceiling_and_floor.get();
          res.position = { query.start.x + query.direction.x * res.distance,
                          query.start.y + query.direction.y * res.distance };
          return res;
        }
        next_h_grid += dy;
      }
    }
    return std::nullopt;
  }

  const std::vector<entity>& world::entities() const {
    return m_entities;
  }

  void world::add_player(const entity& ent) {
    m_player_entity = ent;
    auto position = m_player_entity.position();
    position.x = std::clamp(std::trunc(position.x) + 0.5f, 0.0f, static_cast<float>(m_extent.width - 1));
    position.y = std::clamp(std::trunc(position.y) + 0.5f, 0.0f, static_cast<float>(m_extent.height - 1));
    m_player_entity.move_to(position);
    const auto cell = static_cast<std::int32_t>(position.y) * m_extent.width + static_cast<std::int32_t>(position.x);
    m_cells[cell].flags |= world::CELL_FLAG_SEEN_BIT;
  }

  const entity& world::player() const {
    return m_player_entity;
  }

  entity& world::player() {
    return m_player_entity;
  }

  float world::camera_plane() const {
    return m_camera_plane;
  }

  bool world::walk_player_to(const vec2& position, sdl3::mixer& mixer) {
    auto next = vec2{ std::trunc(position.x) + 0.5f, std::trunc(position.y) + 0.5f };
    if (m_player_entity.mover().is_finished() && length(m_player_entity.position() - next) <= 1.0f)
    {
      const auto index = static_cast<std::int16_t>(next.y) * m_extent.width + static_cast<std::int16_t>(next.x);
      if (index >= 0 && static_cast<std::size_t>(index) < m_cells.size() &&
          !(m_cells[index].flags & (CELL_FLAG_SOLID_BIT | CELL_FLAG_NPC_BIT)))
      {
        m_player_entity.walk_to(next);
        m_time_since_last_turn = stopwatch::duration_type::zero();
        return true;
      }
      else
      {
        mixer.play(world::bump_sfx_default());
        if (m_cells[index].flags & CELL_FLAG_NPC_BIT)
        {
          m_entities[m_cells[index].solid_id - 1].wake_up();
          mixer.play(world::wakeup_sfx_default());
        }
      }
    }
    m_time_since_last_turn = stopwatch::duration_type::zero();
    return false;
  }

  bool world::have_player_attack(sdl3::mixer& mixer) {
    auto target = m_player_entity.position() + m_player_entity.forward();
    target.x = std::trunc(target.x) + 0.5f;
    target.y = std::trunc(target.y) + 0.5f;
    if (m_player_entity.mover().is_finished() && length(m_player_entity.position() - target) <= std::sqrt(2.0f) &&
        m_player_entity.can_attack())
    {
      const auto index = static_cast<std::int16_t>(target.y) * m_extent.width + static_cast<std::int16_t>(target.x);
      if (index >= 0 && static_cast<std::size_t>(index) < m_cells.size() &&
          (m_cells[index].flags & CELL_FLAG_NPC_BIT) && m_cells[index].solid_id)
      {
        m_player_entity.attack();
        auto& enemy = m_entities[m_cells[index].solid_id - 1];
        enemy.damage(1.0f / 3.0f * 100.0f);
        if (!enemy.hp())
        {
          m_cells[index].flags = m_cells[index].flags & ~CELL_FLAG_NPC_BIT;
          m_cells[index].solid_id = 0;
          m_player_entity.add_points(3);
          mixer.play(world::attack_sfx_default());
          mixer.play(world::die_sfx_default());
        }
        else
        {
          mixer.play(world::attack_sfx_default());
          mixer.play(world::hit_sfx_default());
        }
        return true;
      }
    }
    m_time_since_last_turn = stopwatch::duration_type::zero();
    return false;
  }

  void world::rotate_player_by(const float theta) {
    m_player_entity.rotate_by(theta);
  }

  void world::update(const stopwatch::duration_type& dt, sdl3::mixer& mixer) {
    m_player_entity.update(dt);
    if (!m_player_entity.mover().is_finished())
    {
      const auto& position = m_player_entity.position();
      const auto cell = static_cast<std::int32_t>(position.y) * m_extent.width + static_cast<std::int32_t>(position.x);
      m_cells[cell].flags |= world::CELL_FLAG_SEEN_BIT;
      if (m_cells[cell].pickup_id != 0 && m_cells[cell].flags & world::CELL_FLAG_PICKUP_BIT)
      {
        const auto& ent = m_pickups[m_cells[cell].pickup_id - 1];
        if (ent.type() == entity_type::food)
        {
          if (m_player_entity.hp() < 200)
          {
            m_player_entity.heal(ent.hp());
            m_cells[cell].pickup_id = 0;
            m_cells[cell].flags &= ~world::CELL_FLAG_PICKUP_BIT;
          }
        }
        else if (ent.type() == entity_type::treasure)
        {
          if (m_player_entity.score() < 999999)
          {
            m_player_entity.add_points(ent.hp());
            m_cells[cell].pickup_id = 0;
            m_cells[cell].flags &= ~world::CELL_FLAG_PICKUP_BIT;
          }
        }
        mixer.play(world::pickup_sfx_default());
      }
    }
    else
    {
      const auto& position = m_player_entity.position();
      const auto cell = static_cast<std::int32_t>(position.y) * m_extent.width + static_cast<std::int32_t>(position.x);
      if (cell >= 0 && static_cast<std::size_t>(cell) < m_cells.size() &&
          (m_cells[cell].flags & (CELL_FLAG_SOLID_BIT | CELL_FLAG_NPC_BIT)))
      {
        mixer.play(world::bump_sfx_default());
        m_player_entity.walk_to(m_player_entity.mover().origin());
        if (m_cells[cell].flags & CELL_FLAG_NPC_BIT)
        {
          m_entities[m_cells[cell].solid_id - 1].wake_up();
          mixer.play(world::wakeup_sfx_default());
        }
      }
    }
    m_player_light_level = std::fmod(m_player_light_level + 6.0f * tau_v<float> * dt.count() / 8.0f, tau_v<float>);
    static const float HALF_FOV{ std::cos(radians(35.0f)) };
    static const float MAX_DISTANCE{ std::sqrt(8.0f) };
    static const float MAX_ATTACK_DISTANCE{ std::sqrt(2.0f) };
    for (auto& ent : m_entities)
    {
      if (!ent.hp())
      {
        continue;
      }
      if (!ent.is_awake())
      {
        const auto delta = m_player_entity.position() - ent.position();
        const auto distance = length(delta);
        if (distance <= MAX_DISTANCE)
        {
          if (dot(delta, ent.forward()) >= distance * HALF_FOV)
          {
            const auto query = wall_query{ ent.position(), delta / distance, distance };
            const auto hit = wallcast(query);
            if (!hit)
            {
              ent.wake_up();
              mixer.play(world::wakeup_sfx_default());
            }
          }
        }
      }
      else if (ent.is_awake() && m_player_entity.hp())
      {
        const auto delta = m_player_entity.position() - ent.position();
        ent.rotate_to(delta);
        if (ent.can_attack() && ent.mover().is_finished())
        {
          const auto distance = length(delta);
          if (distance <= MAX_ATTACK_DISTANCE)
          {
            const auto query = wall_query{ ent.position(), delta / distance, distance };
            const auto hit = wallcast(query);
            if (!hit)
            {
              m_player_entity.damage(5);
              ent.attack();
              mixer.play(world::attack_sfx_default());
              mixer.play(world::hit_sfx_default());
            }
          }
        }
        if (ent.can_attack() && ent.mover().is_finished())
        {
          const auto target = navigate(ent.position(), m_player_entity.position());
          if (target)
          {
            const auto next = ent.position() + *target;
            if (!is_almost_equal(m_player_entity.position().x, next.x, 1e-6f) ||
                !is_almost_equal(m_player_entity.position().y, next.y, 1e-6f))
            {
              auto index = static_cast<std::size_t>(ent.position().y) * m_extent.width +
                           static_cast<std::size_t>(ent.position().x);
              const auto id = m_cells[index].solid_id;
              m_cells[index].solid_id = 0;
              m_cells[index].flags &= ~CELL_FLAG_NPC_BIT;
              ent.walk_to(next);
              index = static_cast<std::size_t>(next.y) * m_extent.width + static_cast<std::size_t>(next.x);
              m_cells[index].solid_id = id;
              m_cells[index].flags |= CELL_FLAG_NPC_BIT;
            }
          }
        }
      }
      ent.update(dt);
    }
    if (!m_player_entity.hp())
    {
      m_automap = false;
    }
  }

  float world::player_light_level() const {
    return m_player_light_level;
  }

  bool world::is_showing_automap() const {
    return m_automap;
  }

  void world::toggle_automap() {
    m_automap = !m_automap;
  }

  const extent_2d& world::extent() const {
    return m_extent;
  }

  const world::cell& world::at(const offset_2d& coords) const {
    if (coords.x < 0 || coords.x >= m_extent.width || coords.y < 0 || coords.y >= m_extent.height)
    {
      throw error{ error::origin::application, "The application tried to access an out of bounds world cell." };
    }
    return m_cells[coords.y * m_extent.width + coords.x];
  }

  const std::vector<entity>& world::pickups() const {
    return m_pickups;
  }

  const std::vector<entity>& world::enemies() const {
    return m_entities;
  }

  bool world::is_occupied(const offset_2d& cell) const {
    if (cell.x < 0 || cell.x >= m_extent.width || cell.y < 0 || cell.y >= m_extent.height)
    {
      return true;
    }
    const auto index = cell.y * m_extent.width + cell.x;
    if (m_cells[index].flags & (CELL_FLAG_NPC_BIT | CELL_FLAG_SOLID_BIT))
    {
      return true;
    }
    const auto player = offset_2d{ static_cast<std::int16_t>(m_player_entity.position().x),
                                   static_cast<std::int16_t>(m_player_entity.position().y) };
    if (cell.x == player.x && cell.y == player.y)
    {
      return true;
    }
    return false;
  }

  std::optional<vec2> world::navigate(const vec2& from, const vec2& to) const {
    while (!m_navigator.empty())
    {
      m_navigator.pop();
    }
    std::ranges::fill(m_navigator_explored, false);
    const auto root = navigation_node{ { static_cast<std::int16_t>(from.x), static_cast<std::int16_t>(from.y) },
                                       { 0, 0 } };
    const auto goal = offset_2d{ static_cast<std::int16_t>(to.x), static_cast<std::int16_t>(to.y) };
    m_navigator_explored[root.coordinates.y * m_extent.width + root.coordinates.x] = true;
    m_navigator.push(root);
    auto current = navigation_node{ };
    auto next = navigation_node{ };
    auto index = std::int64_t{ };
    while (!m_navigator.empty())
    {
      current = m_navigator.front();
      m_navigator.pop();
      if (current.coordinates == goal)
      {
        return vec2{ static_cast<float>(current.origin.x), static_cast<float>(current.origin.y) };
      }
      next.coordinates.x = current.coordinates.x + 1;
      next.coordinates.y = current.coordinates.y;
      if (!is_occupied(next.coordinates) || next.coordinates == goal)
      {
        index = next.coordinates.y * m_extent.width + next.coordinates.x;
        if (!m_navigator_explored[index])
        {
          m_navigator_explored[index] = true;
          if (current.origin.x == 0 && current.origin.y == 0)
          {
            next.origin.x = 1;
            next.origin.y = 0;
          }
          else
          {
            next.origin = current.origin;
          }
          m_navigator.push(next);
        }
      }
      next.coordinates.x = current.coordinates.x - 1;
      next.coordinates.y = current.coordinates.y;
      if (!is_occupied(next.coordinates) || next.coordinates == goal)
      {
        index = next.coordinates.y * m_extent.width + next.coordinates.x;
        if (!m_navigator_explored[index])
        {
          m_navigator_explored[index] = true;
          if (current.origin.x == 0 && current.origin.y == 0)
          {
            next.origin.x = -1;
            next.origin.y = 0;
          }
          else
          {
            next.origin = current.origin;
          }
          m_navigator.push(next);
        }
      }
      next.coordinates.x = current.coordinates.x;
      next.coordinates.y = current.coordinates.y + 1;
      if (!is_occupied(next.coordinates) || next.coordinates == goal)
      {
        index = next.coordinates.y * m_extent.width + next.coordinates.x;
        if (!m_navigator_explored[index])
        {
          m_navigator_explored[index] = true;
          if (current.origin.x == 0 && current.origin.y == 0)
          {
            next.origin.x = 0;
            next.origin.y = 1;
          }
          else
          {
            next.origin = current.origin;
          }
          m_navigator.push(next);
        }
      }
      next.coordinates.x = current.coordinates.x;
      next.coordinates.y = current.coordinates.y - 1;
      if (!is_occupied(next.coordinates) || next.coordinates == goal)
      {
        index = next.coordinates.y * m_extent.width + next.coordinates.x;
        if (!m_navigator_explored[index])
        {
          m_navigator_explored[index] = true;
          if (current.origin.x == 0 && current.origin.y == 0)
          {
            next.origin.x = 0;
            next.origin.y = -1;
          }
          else
          {
            next.origin = current.origin;
          }
          m_navigator.push(next);
        }
      }
    }
    return std::nullopt;
  }

}
