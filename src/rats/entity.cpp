#include "rats/entity.hpp"

#include <cmath>

#include <algorithm>

#include "rats/error.hpp"
#include "rats/logger.hpp"

namespace {

  float fix_rotation(const float r) {
    auto res = r;
    if (res >= rats::tau_v<float> || res < 0.0f)
    {
      while (res >= rats::tau_v<float>)
      {
        res -= rats::tau_v<float>;
      }
      while (res < 0.0f)
      {
        res += rats::tau_v<float>;
      }
    }
    return res;
  }

}

namespace rats {

  entity::entity() :
  entity{ { 0.0f, 0.0f }, 0.0f, nullptr } { }

  entity::entity(const vec2& position) :
  entity{ position, 0.0f, nullptr } { }

  entity::entity(const vec2& position, const float rotation_about_z) :
  entity{ position, rotation_about_z, nullptr } { }

  entity::entity(const vec2& position, const float rotation_about_z, const std::shared_ptr<assets::texture>& sheet,
                 const entity_type type) :
  m_sheet{ sheet },
  m_attack_cooldown{ 1.0 / 5.0 + (4.0 / 5.0 * (type == entity_type::enemy)) },
  m_damaged_cooldown{ 1.0 / 5.0 },
  m_position{ position },
  m_forward{ std::cos(rotation_about_z), std::sin(rotation_about_z) },
  m_right{ -m_forward.y, m_forward.x },
  m_rotation_about_z{ rotation_about_z },
  m_hp{ 100 },
  m_score{ 0 },
  m_awake{ false },
  m_can_attack{ true },
  m_was_damaged{ false },
  m_type{ type },
  m_mover{ position, position, 0.0 } {
    m_rotation_about_z = fix_rotation(m_rotation_about_z);
    m_max_scan_rotation = fix_rotation(m_rotation_about_z + tau_v<float> / 4.0f);
    m_min_scan_rotation = fix_rotation(m_rotation_about_z - tau_v<float> / 4.0f);
    m_rotation_dir = 1.0f;
    m_forward.x = std::cos(m_rotation_about_z);
    m_forward.y = -std::sin(m_rotation_about_z);
    m_right.x = -m_forward.y;
    m_right.y = m_forward.x;
  }

  void entity::move_to(const vec2& position) {
    m_position = position;
  }

  void entity::walk_to(const vec2& position) {
    m_mover.reset(m_position, position, 0.55);
  }

  const ease_out<vec2>& entity::mover() const {
    return m_mover;
  }

  void entity::rotate_by(const float rotation_about_z) {
    m_rotation_about_z += rotation_about_z;
    m_rotation_about_z = fix_rotation(m_rotation_about_z);
    m_forward.x = std::cos(m_rotation_about_z);
    m_forward.y = -std::sin(m_rotation_about_z);
    m_right.x = -m_forward.y;
    m_right.y = m_forward.x;
  }

  void entity::rotate_to(const vec2& forward) {
    m_forward = forward / length(forward);
    m_right.x = -m_forward.y;
    m_right.y = m_forward.x;
    m_rotation_about_z = fix_rotation(std::acos(m_forward.x));
  }

  float entity::theta() const {
    return m_rotation_about_z;
  }

  const vec2& entity::position() const {
    return m_position;
  }

  const vec2& entity::forward() const {
    return m_forward;
  }

  const vec2& entity::right() const {
    return m_right;
  }

  bool entity::has_texture() const {
    return m_sheet != nullptr;
  }

  const assets::texture& entity::texture() const {
    if (!m_sheet)
    {
      throw error{ error::origin::application, "The entity doesn't have a texture." };
    }
    return *m_sheet;
  }

  void entity::damage(const std::int32_t amount) {
    const auto old = m_hp;
    m_hp = std::clamp(static_cast<std::int64_t>(m_hp) - amount, std::int64_t{ 0 }, std::int64_t{ 200 });
    if (old != m_hp)
    {
      m_was_damaged = true;
      m_damaged_cooldown.clear();
      if (m_type == entity_type::enemy)
      {
        m_awake = true;
      }
    }
  }

  void entity::heal(const std::int32_t amount) {
    m_hp = std::clamp(static_cast<std::int64_t>(m_hp) + amount, std::int64_t{ 0 }, std::int64_t{ 200 });
  }

  void entity::set_hp_to(const std::uint32_t hp) {
    m_hp = std::clamp(hp, std::uint32_t{ 0 }, std::uint32_t{ 200 });
  }

  void entity::decay_hp() {
    if (m_hp > 100)
    {
      const auto take = std::min(m_hp - 100, 5U);
      m_hp -= take;
    }
  }

  std::uint32_t entity::hp() const {
    return m_hp;
  }

  void entity::update(const stopwatch::duration_type& dt) {
    if (m_type == entity_type::player || m_type == entity_type::enemy)
    {
      if (!m_can_attack)
      {
        m_attack_cooldown.update(dt);
        while (m_attack_cooldown.consume())
        {
          m_can_attack = true;
          m_attack_cooldown.clear();
        }
      }
      if (m_was_damaged)
      {
        m_damaged_cooldown.update(dt);
        while (m_damaged_cooldown.consume())
        {
          m_was_damaged = false;
          m_damaged_cooldown.clear();
        }
      }
      if (!m_mover.is_finished())
      {
        m_mover.update(dt);
        move_to(m_mover());
      }
    }
    if (m_type == entity_type::enemy)
    {
      if (!m_awake)
      {
        rotate_by(tau_v<float> / 4.0f * m_rotation_dir * dt.count());
        if (m_rotation_about_z >= m_max_scan_rotation || m_rotation_about_z <= m_min_scan_rotation)
        {
          m_rotation_dir = std::trunc(m_rotation_dir * -1.0f);
        }
      }
    }
  }

  void entity::attack() {
    if (m_can_attack)
    {
      m_can_attack = false;
    }
  }

  bool entity::can_attack() const {
    return m_can_attack;
  }

  bool entity::was_hit() const {
    return m_was_damaged;
  }

  bool entity::is_awake() const {
    return m_awake;
  }

  void entity::wake_up() {
    m_awake = true;
  }

  entity_type entity::type() const {
    return m_type;
  }

  void entity::add_points(const std::uint64_t points) {
    m_score = std::clamp(m_score + points, std::uint64_t{ 0 }, std::uint64_t{ 999999 });
  }

  std::uint64_t entity::score() const {
    return m_score;
  }

}
