#ifndef RATS_ENTITY_HPP
#define RATS_ENTITY_HPP

#include <cinttypes>

#include <memory>

#include "math.hpp"
#include "timer.hpp"
#include "stopwatch.hpp"
#include "easing.hpp"

namespace rats::assets {

  class texture;

}

namespace rats {

  enum class entity_type {
    unknown,
    player,
    enemy,
    food,
    treasure
  };

  class entity final {
  private:
    std::shared_ptr<assets::texture> m_sheet;
    timer m_attack_cooldown;
    timer m_damaged_cooldown;
    vec2 m_position;
    vec2 m_forward;
    vec2 m_right;
    float m_rotation_about_z;
    float m_max_scan_rotation;
    float m_min_scan_rotation;
    float m_rotation_dir;
    std::uint64_t m_id;
    std::uint32_t m_hp;
    std::uint64_t m_score;
    bool m_awake;
    bool m_can_attack;
    bool m_was_damaged;
    entity_type m_type;
    ease_out<vec2> m_mover;
  public:
    entity();
    explicit entity(const vec2& position);
    entity(const vec2& position, const float rotation_about_z);
    entity(const vec2& position, const float rotation_about_z, const std::shared_ptr<assets::texture>& sheet,
           const entity_type type = entity_type::unknown);
    entity(const entity& other) = default;
    entity(entity&& other) = default;

    ~entity() noexcept = default;

    entity& operator=(const entity& rhs) = default;
    entity& operator=(entity&& rhs) = default;

    void move_to(const vec2& position);
    void walk_to(const vec2& position);
    const ease_out<vec2>& mover() const;
    void rotate_by(const float rotation_about_z);
    void rotate_to(const vec2& forward);
    float theta() const;
    const vec2& position() const;
    const vec2& forward() const;
    const vec2& right() const;
    bool has_texture() const;
    const assets::texture& texture() const;
    void damage(const std::int32_t amount);
    void heal(const std::int32_t amount);
    void set_hp_to(const std::uint32_t hp);
    void decay_hp();
    std::uint32_t hp() const;
    void update(const stopwatch::duration_type& dt);
    void attack();
    bool can_attack() const;
    bool was_hit() const;
    bool is_awake() const;
    void wake_up();
    entity_type type() const;
    std::uint64_t score() const;
    void add_points(const std::uint64_t points);
  };

}

#endif
